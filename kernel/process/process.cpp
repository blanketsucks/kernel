#include <kernel/process/process.h>
#include <kernel/process/threads.h>
#include <kernel/process/scheduler.h>

#include <kernel/posix/unistd.h>
#include <kernel/posix/sys/mman.h>

#include <std/format.h>

namespace kernel {

Process* Process::create_kernel_process(String name, void (*entry)()) {
    return new Process(1, move(name), true, entry);
}

Process* Process::create_user_process(String name, ELF elf, RefPtr<fs::ResolvedInode> cwd, ProcessArguments&& arguments, TTY* tty) {
    auto* process = new Process(generate_id(), move(name), false, nullptr, cwd, move(arguments), tty);
    process->create_user_entry(elf);

    return process;
}

Process::Process(
    pid_t id, String name, bool kernel, void (*entry)(), RefPtr<fs::ResolvedInode> cwd, ProcessArguments arguments, TTY* tty
) : m_id(id), m_name(move(name)), m_kernel(kernel), m_tty(tty), m_cwd(cwd), m_arguments(move(arguments)) {
    if (!kernel) {
        m_page_directory = arch::PageDirectory::create_user_page_directory();
        m_allocator = memory::RegionAllocator({ PAGE_SIZE, KERNEL_VIRTUAL_BASE - PAGE_SIZE }, m_page_directory);
    } else {
        m_page_directory = arch::PageDirectory::kernel_page_directory();
        
        auto thread = Thread::create(m_id, "main", this, entry, m_arguments);
        this->add_thread(thread);
    }

    if (m_tty) {
        // Set up stdin, stdout, and stderr
        auto fd = tty->open(O_RDWR);
        for (size_t i = 0; i < 3; i++) {
            m_file_descriptors.append(fd);
        }
    }
}

void Process::create_user_entry(ELF elf) {
    elf.load();
    if (elf.has_interpreter()) {
        auto* vfs = fs::vfs();
        auto file = vfs->open(elf.interpreter(), O_RDONLY, 0, m_cwd).unwrap();

        elf = ELF(file);
        elf.load();

        Vector<String> argv = m_arguments.argv;

        m_arguments.argv = { elf.interpreter() };
        m_arguments.argv.extend(move(argv));
    }

    m_page_directory->switch_to(); // FIXME: Should we do this?

    auto& file = elf.file();
    for (auto& ph : elf.program_headers()) {
        if (ph.p_type != PT_LOAD || ph.p_vaddr == 0) {
            continue;
        }

        uintptr_t address = std::align_down(ph.p_vaddr, PAGE_SIZE);
        size_t size = std::align_up(ph.p_memsz, PAGE_SIZE);
        
        u8* region = reinterpret_cast<u8*>(this->allocate_at(address, size, PageFlags::Write));

        file.seek(ph.p_offset, SEEK_SET);
        file.read(region + (ph.p_vaddr - address), ph.p_filesz);
    }

    auto entry = reinterpret_cast<void(*)()>(elf.entry());

    auto* dir = arch::PageDirectory::kernel_page_directory();
    dir->switch_to();
    
    auto* thread = Thread::create(m_id, "main", this, entry, m_arguments);
    this->add_thread(thread);
}

Process* Process::fork() {
    return nullptr;
}

void Process::add_thread(Thread* thread) {
    m_threads.set(thread->id(), thread);
}

void Process::remove_thread(Thread* thread) {
    m_exit_values.set(thread->id(), thread->m_exit_value);
    m_threads.remove(thread->id());
}

Thread* Process::get_thread(u32 id) const {
    auto iterator = m_threads.find(id);
    if (iterator != m_threads.end()) {
        return iterator->value;
    }

    return nullptr;
}

Thread* Process::get_main_thread() const {
    return this->get_thread(m_id);
}

Thread* Process::spawn(String name, void (*entry)()) {
    auto thread = Thread::create(generate_id(), move(name), this, entry, m_arguments);
    this->add_thread(thread);

    Scheduler::queue(thread);
    return thread;
}

void* Process::allocate(size_t size, PageFlags flags) {
    if (this->is_kernel()) {
        return MM->allocate_kernel_region(size);
    }

    return MM->allocate(m_allocator, size, flags | PageFlags::User);
}

void* Process::allocate_at(VirtualAddress address, size_t size, PageFlags flags) {
    if (this->is_kernel()) {
        return MM->allocate_kernel_region(size);
    }

    return MM->allocate_at(m_allocator, address, size, flags | PageFlags::User);
}

void* Process::allocate_with_physical_region(PhysicalAddress address, size_t size, int prot) {
    ASSERT(address % PAGE_SIZE == 0, "Physical address must be page aligned.");
    auto* region = m_allocator.allocate(size, prot);
    if (!region) {
        return nullptr;
    }

    PageFlags pflags = PageFlags::User;
    if (prot & PROT_WRITE) {
        pflags |= PageFlags::Write;
    }

    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        m_page_directory->map(region->base() + i, address + i, pflags);
    }

    return reinterpret_cast<void*>(region->base());
}

void Process::notify_exit(Thread* thread) {
    this->remove_thread(thread);
}

void Process::handle_page_fault(arch::InterruptRegisters* regs, VirtualAddress address) {
    // FIXME: Send proper signal once that is implemented
    memory::PageFault fault = regs->errno;

    auto* region = m_allocator.find_region(address, true);
    if (!region || !region->is_file_backed()) {
        dbgln("\033[1;31mPage fault (address={:#p}) at EIP={:#p} ({}{}{}):\033[0m", address, regs->eip, fault.present ? 'P' : '-', fault.rw ? 'W' : 'R', fault.user ? 'U' : 'S');
        dbgln("  \033[1;31mUnrecoverable page fault.\033[0m", m_name.data(), m_id);

        this->kill();
    }

    size_t offset = address - region->base();
    size_t index = std::align_down(offset, PAGE_SIZE);

    auto* file = region->file();

    void* frame = MM->allocate_physical_frame();
    m_page_directory->map(region->base() + index, reinterpret_cast<PhysicalAddress>(frame), PageFlags::Write | PageFlags::User);

    file->read(reinterpret_cast<void*>(region->base() + index), PAGE_SIZE, index);
}

memory::Region* Process::validate_pointer_access(const void* ptr, bool write) {
    auto* region = m_allocator.find_region(reinterpret_cast<VirtualAddress>(ptr), true);
    if (!region) {
        dbgln("Invalid memory access at {:#}. Killing.", m_id, ptr);
        this->kill();
    } else if (write && !region->is_writable()) {
        dbgln("Invalid memory access at {:#}. Killing.", m_id, ptr);
        this->kill();
    } 
    
    return region;
}

void Process::validate_pointer_access(const void* ptr, size_t size, bool write) {
    memory::Region* region = nullptr;
    if (size <= PAGE_SIZE) {
        region = this->validate_pointer_access(ptr, write);
    } else {
        size_t i = 0;
        for (; i < size; i += PAGE_SIZE) {
            region = this->validate_pointer_access(reinterpret_cast<const u8*>(ptr) + i, write);
        }
    }

    size_t offset = reinterpret_cast<VirtualAddress>(ptr) - region->base();
    if (offset + size > region->size()) {
        dbgln("Invalid memory access at {:#}. Killing.", m_id, ptr);
        this->kill();
    }
}

void Process::validate_read(const void* ptr, size_t size) {
    this->validate_pointer_access(ptr, size, false);
}

void Process::validate_write(const void* ptr, size_t size) {
    this->validate_pointer_access(ptr, size, true);
}

RefPtr<fs::FileDescriptor> Process::get_file_descriptor(int fd) {
    if (fd < 0 || static_cast<size_t>(fd) >= m_file_descriptors.size()) {
        return nullptr;
    }

    return m_file_descriptors[fd];
}

void Process::kill() {
    m_state = Dead;
    for (auto& [_, thread] : m_threads) {
        thread->kill();
    }

    Scheduler::yield();
}

void Process::sys$exit(int status) {
    dbgln("Process exited with status {}.", status);

    m_exit_status = status;
    this->kill();
}

int Process::sys$open(const char* path, size_t path_length, int flags, mode_t mode) {
    this->validate_read(path, path_length);
    auto vfs = fs::vfs();

    auto result = vfs->open({ path, path_length }, flags, mode, m_cwd);
    if (result.is_err()) {
        return -result.error().errno();
    }

    auto fd = result.value();
    m_file_descriptors.append(move(fd));

    return m_file_descriptors.size() - 1;
}

int Process::sys$close(int fd) {
    if (fd < 0 || static_cast<size_t>(fd) >= m_file_descriptors.size()) {
        return -EBADF;
    }

    m_file_descriptors[fd] = nullptr;
    return 0;
}

ssize_t Process::sys$read(int fd, void* buffer, size_t size) {
    auto file = this->get_file_descriptor(fd);
    if (!file) {
        return -EBADF;
    }

    this->validate_read(buffer, size);
    return file->read(buffer, size);
}

ssize_t Process::sys$write(int fd, const void* buffer, size_t size) {
    auto file = this->get_file_descriptor(fd);
    if (!file) {
        return -EBADF;
    }

    this->validate_write(buffer, size);
    return file->write(buffer, size);
}

int Process::sys$fstat(int fd, stat* buffer) {
    auto file = this->get_file_descriptor(fd);

    this->validate_write(buffer, sizeof(stat));
    *buffer = file->stat();

    return 0;
}

int Process::sys$dup(int old_fd) {
    auto file = this->get_file_descriptor(old_fd);
    if (!file) {
        return -EBADF;
    }

    int new_fd = -1;
    for (size_t i = 0; i < m_file_descriptors.size(); i++) {
        if (!m_file_descriptors[i]) {
            new_fd = i;
            break;
        }
    }

    if (new_fd == -1) {
        m_file_descriptors.append(file);
        new_fd = m_file_descriptors.size() - 1;
    } else {
        m_file_descriptors[new_fd] = file;
    }

    return new_fd;
}

int Process::sys$dup2(int old_fd, int new_fd) {
    auto file = this->get_file_descriptor(old_fd);

    if (new_fd < 0 || static_cast<size_t>(new_fd) >= m_file_descriptors.size()) {
        return -EBADF;
    }

    m_file_descriptors[new_fd] = file;
    return new_fd;
}

void* Process::sys$mmap(void*, size_t size, int prot, int flags, int fileno, off_t) {
    size = std::align_up(size, PAGE_SIZE);

    PageFlags pflags = PageFlags::None;
    if (prot & PROT_WRITE) {
        pflags |= PageFlags::Write;
    }

    // TODO: Handle first argument and MAP_FIXED

    void* address = nullptr;
    if (flags & MAP_ANONYMOUS) {
        address = this->allocate(size, pflags);
    } else {
        if (fileno < 0 || static_cast<size_t>(fileno) >= m_file_descriptors.size()) {
            return MAP_FAILED;
        }

        auto& fd = m_file_descriptors[fileno];
        if (!fd) {
            return MAP_FAILED;
        }

        auto result = fd->mmap(*this, size, prot);
        if (result.is_err()) {
            return MAP_FAILED;
        }

        address = result.value();
    }

    auto* region = m_allocator.find_region(address);
    region->set_prot(prot);

    return address;
}

int Process::sys$getcwd(char* buffer, size_t size) {
    this->validate_write(buffer, size);

    auto cwd = m_cwd;
    if (!cwd) {
        cwd = fs::vfs()->resolve("/").unwrap();
    }

    auto path = cwd->fullpath();
    if (path.size() > size) {
        return -ERANGE;
    }

    memcpy(buffer, path.data(), path.size());
    buffer[path.size()] = '\0';

    return 0;
}

int Process::sys$chdir(const char* path, size_t path_length) {
    this->validate_read(path, path_length);

    auto vfs = fs::vfs();
    auto result = vfs->resolve({ path, path_length }, m_cwd);
    if (result.is_err()) {
        return -result.error().errno();
    }

    m_cwd = result.value();
    return 0;
}

int Process::sys$ioctl(int fd, unsigned request, unsigned arg) {
    auto file = this->get_file_descriptor(fd);
    if (!file) {
        return -EBADF;
    }

    return file->ioctl(request, arg);
}

}