#include <kernel/process/scheduler.h>
#include <kernel/process/process.h>
#include <kernel/process/syscalls.h>
#include <kernel/process/threads.h>
#include <kernel/process/blocker.h>
#include <kernel/memory/manager.h>
#include <kernel/posix/sys/mman.h>
#include <kernel/posix/sys/wait.h>
#include <kernel/time/manager.h>
#include <kernel/posix/unistd.h>
#include <kernel/arch/cpu.h>
#include <kernel/arch/interrupts.h>

#include <std/format.h>

namespace kernel {

Process* Process::create_kernel_process(String name, void (*entry)(void*), void* data) {
    return new Process(generate_id(), move(name), true, entry, data);
}

ErrorOr<Process*> Process::create_user_process(String path, RefPtr<fs::ResolvedInode> cwd, TTY* tty) {
    auto vfs = fs::vfs();
    auto file = TRY(vfs->open(path, O_RDONLY, 0, cwd));

    ELF elf(file);

    ProcessArguments arguments;
    arguments.argv = { path };

    auto* process = new Process(generate_id(), move(path), false, nullptr, nullptr, cwd, move(arguments), tty);
    TRY(process->create_user_entry(elf));

    return process;
}

ErrorOr<Process*> Process::create_user_process(String name, ELF elf, RefPtr<fs::ResolvedInode> cwd, ProcessArguments arguments, TTY* tty) {
    auto* process = new Process(generate_id(), move(name), false, nullptr, nullptr, cwd, move(arguments), tty);
    TRY(process->create_user_entry(elf));

    return process;
}

Process* Process::current() {
    return Scheduler::current_process();
}

Process::Process(
    pid_t id,
    String name,
    bool kernel,
    void (*entry)(void*),
    void* entry_data,
    RefPtr<fs::ResolvedInode> cwd,
    ProcessArguments arguments,
    TTY* tty,
    Process* parent
) : m_state(Alive), m_id(id), m_name(move(name)), m_kernel(kernel), m_tty(tty), m_cwd(cwd), m_arguments(move(arguments)) {
    if (!kernel) {
        m_page_directory = arch::PageDirectory::create_user_page_directory();
        if (!parent) {
            m_allocator = memory::RegionAllocator::create(
                { PAGE_SIZE, static_cast<size_t>(g_boot_info->kernel_virtual_base - PAGE_SIZE) }, m_page_directory
            );
        }
    } else {
        m_page_directory = arch::PageDirectory::kernel_page_directory();

        auto thread = Thread::create(m_id, "main", this, entry, entry_data, m_arguments);
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

ErrorOr<void> Process::create_user_entry(ELF elf) {
    TRY(elf.load());

    if (elf.has_interpreter()) {
        auto* vfs = fs::vfs();
        auto file = TRY(vfs->open(elf.interpreter(), O_RDONLY, 0, m_cwd));

        elf = ELF(file);
        TRY(elf.load());

        Vector<String> argv = m_arguments.argv;

        m_arguments.argv = { elf.interpreter() };
        m_arguments.argv.extend(move(argv));
    }

    auto& file = elf.file();
    for (auto& ph : elf.program_headers()) {
        if (ph.p_type != PT_LOAD || ph.p_vaddr == 0) {
            continue;
        }

        uintptr_t address = std::align_down(ph.p_vaddr, PAGE_SIZE);
        size_t size = std::align_up(ph.p_memsz, PAGE_SIZE);

        u8* region = reinterpret_cast<u8*>(TRY(this->allocate_at(address, size, PageFlags::Write)));
        memory::TemporaryMapping temp(*m_page_directory, region, size);

        file.seek(ph.p_offset, SEEK_SET);
        TRY(file.read(temp.ptr() + (ph.p_vaddr - address), ph.p_filesz));
    }

    auto entry = reinterpret_cast<void(*)(void*)>(elf.entry());
    
    auto* thread = Thread::create(m_id, "main", this, entry, nullptr, m_arguments);
    this->add_thread(thread);

    return {};
}

Process* Process::fork(arch::Registers& registers) {
    auto* process = new Process(generate_id(), m_name, false, nullptr, nullptr, nullptr, {}, m_tty);

    process->m_parent_id = m_id;
    process->m_allocator = m_allocator->clone(process->page_directory());

    process->m_file_descriptors.resize(m_file_descriptors.size());
    process->m_cwd = m_cwd;

    for (size_t i = 0; i < m_file_descriptors.size(); i++) {
        process->m_file_descriptors[i] = m_file_descriptors[i];
    }

    auto* thread = new Thread(process, registers);
    process->add_thread(thread);

    return process;
}

void Process::add_thread(Thread* thread) {
    m_threads.set(thread->id(), thread);
}

void Process::remove_thread(Thread* thread) {
    m_exit_values.set(thread->id(), thread->m_exit_value);
    m_threads.remove(thread->id());
}

Thread* Process::get_thread(pid_t id) const {
    auto iterator = m_threads.find(id);
    if (iterator != m_threads.end()) {
        return iterator->value;
    }

    return nullptr;
}

Thread* Process::get_main_thread() const {
    return this->get_thread(m_id);
}

Thread* Process::spawn(String name, void (*entry)(void*), void* data) {
    auto thread = Thread::create(generate_id(), move(name), this, entry, data, m_arguments);
    this->add_thread(thread);

    Scheduler::queue(thread);
    return thread;
}

ErrorOr<void*> Process::allocate(size_t size, PageFlags flags) {
    if (this->is_kernel()) {
        return MM->allocate_kernel_region(size);
    }

    return MM->allocate(*m_allocator, size, flags | PageFlags::User);
}

ErrorOr<void*> Process::allocate_at(VirtualAddress address, size_t size, PageFlags flags) {
    if (this->is_kernel()) {
        return MM->allocate_kernel_region(size);
    }

    return MM->allocate_at(*m_allocator, address, size, flags | PageFlags::User);
}

ErrorOr<void*> Process::allocate_from_kernel_region(VirtualAddress address, size_t size, int prot) {
    ASSERT(address % PAGE_SIZE == 0, "address must be page aligned.");
    auto* region = m_allocator->allocate(size, prot);
    if (!region) {
        return Error(ENOMEM);
    }

    PageFlags pflags = PageFlags::User;
    if (prot & PROT_WRITE) {
        pflags |= PageFlags::Write;
    }

    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        PhysicalAddress pa = MM->get_physical_address(reinterpret_cast<void*>(address + i));
        m_page_directory->map(region->base() + i, pa, pflags);
    }

    region->set_kernel_managed(true);
    return reinterpret_cast<void*>(region->base());
}

ErrorOr<void*> Process::allocate_with_physical_region(PhysicalAddress address, size_t size, int prot) {
    ASSERT(address % PAGE_SIZE == 0, "address must be page aligned.");
    auto* region = m_allocator->allocate(size, prot);
    if (!region) {
        return Error(ENOMEM);
    }

    PageFlags pflags = PageFlags::User;
    if (prot & PROT_WRITE) {
        pflags |= PageFlags::Write;
    }

    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        m_page_directory->map(region->base() + i, address + i, pflags);
    }

    region->set_kernel_managed(true);
    return reinterpret_cast<void*>(region->base());
}

ErrorOr<void*> Process::allocate_file_backed_region(fs::File* file, size_t size) {
    ASSERT(size % PAGE_SIZE == 0, "size must be page aligned");
    auto* region = m_allocator->create_file_backed_region(file, size);
    if (!region) {
        return Error(ENOMEM);
    }

    return reinterpret_cast<void*>(region->base());
}

void Process::notify_exit(Thread* thread) {
    this->remove_thread(thread);
}

void Process::handle_page_fault(arch::InterruptRegisters* regs, VirtualAddress address) {
    // TODO: Send proper signal once that is implemented
    memory::PageFault fault = regs->errno;
    bool is_null_pointer_dereference = address < PAGE_SIZE;

    auto* region = m_allocator->find_region(address, true);
    if (region && fault.rw && fault.present) {
        address = std::align_down(address, PAGE_SIZE);
        arch::PageTableEntry* entry = m_page_directory->get_page_table_entry(address);

        auto* page = MM->get_physical_page(entry->physical_address());
        if (!page) {
            goto unrecoverable_fault;
        }

        bool is_cow = page->flags & memory::PhysicalPage::CoW;
        if (!is_cow) {
            goto unrecoverable_fault;
        }

        if (page->ref_count > 1) {
            page->ref_count--;

            void* frame = MUST(MM->allocate_page_frame());

            // TODO: Move this to a function in MemoryManager
            page = MM->get_physical_page(reinterpret_cast<PhysicalAddress>(frame));
            page->ref_count++;

            MM->copy_physical_memory(frame, reinterpret_cast<void*>(entry->physical_address()), PAGE_SIZE);
            entry->set_physical_address(reinterpret_cast<PhysicalAddress>(frame));
        } else {
            page->flags &= ~memory::PhysicalPage::CoW;
        }

        entry->set_writable(true);
        arch::invlpg(address);

        return;
    }

    if (!region || !region->is_file_backed() || !region->used()) {        
unrecoverable_fault:
        if (is_null_pointer_dereference) {
            dbgln("\033[1;31mNull pointer dereference (address={:#p}) @ IP={:#p} ({}{}{}):\033[0m", address, regs->ip(), fault.present ? 'P' : '-', fault.rw ? 'W' : 'R', fault.user ? 'U' : 'S');
        } else {
            dbgln("\033[1;31mPage fault (address={:#p}) @ IP={:#p} ({}{}{}):\033[0m", address, regs->ip(), fault.present ? 'P' : '-', fault.rw ? 'W' : 'R', fault.user ? 'U' : 'S');
        }

        StringView message;
        if (fault.rsvd) {
            message = "Reserved bit set in page table entry.";
        } else if (!fault.present) {
            message = "Page not present.";
        } else if (fault.id && !region->is_executable()) {
            message = "Attempt to execute non-executable region.";
        } else if (!fault.rw && !region->is_readable()) {
            message = "Attempt to read from non-readable region.";
        } else if (fault.rw && !region->is_writable()) {
            message = "Attempt to write to non-writable region.";
        }

        if (!message.empty()) {
            dbgln("  \033[1;31mUnrecoverable page fault: {}\033[0m", message);
        } else {
            dbgln("  \033[1;31mUnrecoverable page fault.\033[0m");
        }

        dbgln("Process memory regions:");
        m_allocator->for_each_region([](memory::Region* region) {
            if (!region->used()) {
                return;
            }

            dbgln("  {:#p} - {:#p} ({}{}{})", region->base(), region->end(), region->is_writable() ? 'W' : 'R', region->is_executable() ? 'X' : '-', region->is_shared() ? 'S' : 'P');
        });

        dbgln();
        this->kill();
    }

    size_t offset = (address - region->base()) + region->offset();
    auto* file = region->file();

    size_t index = std::align_down(offset, PAGE_SIZE);

    void* frame = MUST(MM->allocate_page_frame());
    m_page_directory->map(region->base() + index, reinterpret_cast<PhysicalAddress>(frame), PageFlags::Write | PageFlags::User);

    size_t size = std::min(file->size() - index, PAGE_SIZE);
    auto result = file->read(reinterpret_cast<void*>(region->base() + index), size, index);

    if (result.is_err()) {
        dbgln("\033[1;31mFailed to read file backed region (address={:#p}) @ IP={:#p}:\033[0m", address, regs->ip());
        dbgln("  \033[1;31merrno={}\033[0m", result.error().code());

        this->kill();
    }
}

void Process::handle_general_protection_fault(arch::InterruptRegisters* regs) {
    dbgln("\033[1;31mGeneral protection fault at IP={:#p}:\033[0m", regs->ip());
    dbgln("  \033[1;31mError code: {:#x}\033[0m", regs->errno);

    dbgln();

    this->kill();
}

memory::Region* Process::validate_pointer_access(const void* ptr, bool write) {
    auto* region = m_allocator->find_region(reinterpret_cast<VirtualAddress>(ptr), true);

    if (!region) {
        dbgln("Invalid memory access @ {:#}. Killing.", ptr);
        this->kill();
    } else if (write && !region->is_writable()) {
        dbgln("Write to read-only region @ {:#}. Killing.", ptr);
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
        dbgln("Invalid memory access at {:#}. Killing.", ptr);
        this->kill();
    }
}

void Process::validate_read(const void* ptr, size_t size) {
    this->validate_pointer_access(ptr, size, false);
}

void Process::validate_write(const void* ptr, size_t size) {
    this->validate_pointer_access(ptr, size, true);
}

StringView Process::validate_string(const char* ptr) {
    memory::Region* region = this->validate_pointer_access(ptr, false);
    size_t offset = reinterpret_cast<VirtualAddress>(ptr) - region->base();

    size_t length = 0;
    while (*ptr) {
        // The string could be split across multiple memory regions. I don't know how likely this is to happen in practice.
        if (offset >= region->size()) {
            region = this->validate_pointer_access(ptr, false);
            offset = 0;
        }

        ptr++;
        offset++;

        length++;
    }

    return { ptr - length, length };
}

RefPtr<fs::FileDescriptor> Process::get_file_descriptor(int fd) {
    if (fd < 0 || static_cast<size_t>(fd) >= m_file_descriptors.size()) {
        return nullptr;
    }

    return m_file_descriptors[fd];
}

void Process::kill() {
    for (auto& [_, thread] : m_threads) {
        thread->kill();
    }
    
    for (auto& fd : m_file_descriptors) {
        fd.~RefPtr();
    }
    
    m_state = Dead;
    if (!m_allocator) {
        Scheduler::yield();
        return;
    }

    m_allocator->for_each_region([this](auto* region) {
        if (region->is_file_backed()) {
            // TODO
            return;
        } else if (!region->used() || region->is_kernel_managed()) {
            return;
        }

        MM->free(m_allocator->page_directory(), region->base(), region->size());
        m_allocator->free(region);
    });

    Scheduler::yield();
}

void Process::sys$exit(int status) {
    dbgln("Process exited with status {}.", status);

    m_exit_status = status;
    WaitBlocker::try_wake_all(this, status);

    this->kill();
    __builtin_unreachable();
}

ErrorOr<FlatPtr> Process::sys$getpid() { return m_id; }
ErrorOr<FlatPtr> Process::sys$getppid() { return m_parent_id; }
ErrorOr<FlatPtr> Process::sys$gettid() { return Thread::current()->id(); }

ErrorOr<FlatPtr> Process::sys$open(const char* pathname, int flags, mode_t mode) {
    StringView path = this->validate_string(pathname);
    auto vfs = fs::vfs();

    auto fd = TRY(vfs->open(path, flags, mode, m_cwd));

    m_file_descriptors.append(move(fd));
    return m_file_descriptors.size() - 1;
}

ErrorOr<FlatPtr> Process::sys$close(int fd) {
    if (fd < 0 || static_cast<size_t>(fd) >= m_file_descriptors.size()) {
        return Error(EBADF);
    }

    auto& file = m_file_descriptors[fd];
    if (file) {
        file.~RefPtr();
    }

    m_file_descriptors[fd] = nullptr;
    return 0;
}

ErrorOr<FlatPtr> Process::sys$read(int fd, void* buffer, size_t size) {
    auto file = this->get_file_descriptor(fd);
    if (!file) {
        return Error(EBADF);
    }

    this->validate_read(buffer, size);
    return file->read(buffer, size);
}

ErrorOr<FlatPtr> Process::sys$write(int fd, const void* buffer, size_t size) {
    auto file = this->get_file_descriptor(fd);
    if (!file) {
        return Error(EBADF);
    }

    this->validate_write(buffer, size);
    return file->write(buffer, size);
}

ErrorOr<FlatPtr> Process::sys$fstat(int fd, stat* buffer) {
    auto file = this->get_file_descriptor(fd);

    this->validate_write(buffer, sizeof(stat));
    *buffer = file->stat();

    return 0;
}

ErrorOr<FlatPtr> Process::sys$lseek(int fd, off_t offset, int whence) {
    auto file = this->get_file_descriptor(fd);
    if (!file) {
        return Error(EBADF);
    }

    file->seek(offset, whence);
    return file->offset();
}

ErrorOr<FlatPtr> Process::sys$dup(int old_fd) {
    auto file = this->get_file_descriptor(old_fd);
    if (!file) {
        return Error(EBADF);
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

ErrorOr<FlatPtr> Process::sys$dup2(int old_fd, int new_fd) {
    auto file = this->get_file_descriptor(old_fd);

    if (new_fd < 0 || static_cast<size_t>(new_fd) >= m_file_descriptors.size()) {
        return Error(EBADF);
    }

    m_file_descriptors[new_fd] = file;
    return new_fd;
}

ErrorOr<FlatPtr> Process::sys$mmap(mmap_args* args) {
    this->validate_pointer_access(args, sizeof(mmap_args), false);

    size_t size = args->size;
    VirtualAddress hint = reinterpret_cast<VirtualAddress>(args->addr);
    int prot = args->prot;
    int flags = args->flags;
    int fileno = args->fd;

    size = std::align_up(size, PAGE_SIZE);

    PageFlags pflags = PageFlags::NoExecute;
    if (prot & PROT_WRITE) {
        pflags |= PageFlags::Write;
    } 

    if (prot & PROT_EXEC) {
        pflags &= ~PageFlags::NoExecute;
    }

    void* address = nullptr;
    if (flags & MAP_ANONYMOUS) {
        if (flags & MAP_FIXED) {
            address = TRY(this->allocate_at(hint, size, pflags));
        } else {
            address = TRY(this->allocate(size, pflags));
        }
    } else {
        if (fileno < 0 || static_cast<size_t>(fileno) >= m_file_descriptors.size()) {
            return Error(EBADF);
        }

        auto& fd = m_file_descriptors[fileno];
        if (!fd) {
            return Error(EBADF);
        }

        address = TRY(fd->mmap(*this, size, prot));
    }

    auto* region = m_allocator->find_region(reinterpret_cast<VirtualAddress>(address), true);

    region->set_prot(prot);
    region->set_offset(args->offset);

    if (flags & MAP_SHARED) {
        region->set_shared(true);
    }

    return (FlatPtr)address;
}

ErrorOr<FlatPtr> Process::sys$munmap(FlatPtr address, size_t size) {
    if (address % PAGE_SIZE != 0) {
        return Error(EINVAL);
    } else if (size % PAGE_SIZE != 0) {
        return Error(EINVAL);
    }

    auto* region = m_allocator->find_region(reinterpret_cast<VirtualAddress>(address), true);
    if (!region) {
        return Error(EINVAL);
    }

    if (size > region->size()) {
        return Error(EINVAL);
    } else if (region->end() < address + size) {
        return Error(EINVAL);
    }

    if (region->base() == address) {
        if (region->size() == size) {
            MM->free(m_allocator->page_directory(), region->base(), region->size());
            m_allocator->free(region);

            return 0;
        }

        region->set_range({ region->base() + size, region->size() - size });
        MM->free(m_page_directory, address, size);

        return 0;
    } else {
        if (region->end() == address + size) {
            region->set_range({ region->base(), region->size() - size });
            MM->free(m_page_directory, address, size);

            return 0;
        }

        auto* new_region = memory::Region::create(address + size, region->end() - (address + size));
        new_region->set_prot(region->prot());
        new_region->set_shared(region->is_shared());

        region->set_range({ region->base(), address - region->base() });

        m_allocator->insert_after(region, new_region);
        MM->free(m_page_directory, address, size);
    }

    return 0;
}

ErrorOr<FlatPtr> Process::sys$getcwd(char* buffer, size_t size) {
    this->validate_write(buffer, size);

    auto cwd = m_cwd;
    if (!cwd) {
        cwd = fs::vfs()->resolve("/").unwrap();
    }

    auto path = cwd->fullpath();
    if (path.size() > size) {
        return Error(ERANGE);
    }

    std::memcpy(buffer, path.data(), path.size());
    buffer[path.size()] = '\0';

    return 0;
}

ErrorOr<FlatPtr> Process::sys$chdir(const char* pathname) {
    StringView path = this->validate_string(pathname);

    auto vfs = fs::vfs();
    m_cwd = TRY(vfs->resolve(path, nullptr, m_cwd));

    return 0;
}

ErrorOr<FlatPtr> Process::sys$readdir(int fd, void* buffer, size_t size) {
    auto file = this->get_file_descriptor(fd);
    if (!file) {
        return Error(EBADF);
    }

    return file->file()->readdir(buffer, size);
}

ErrorOr<FlatPtr> Process::sys$ioctl(int fd, unsigned request, unsigned arg) {
    auto file = this->get_file_descriptor(fd);
    if (!file) {
        return Error(EBADF);
    }

    return TRY(file->ioctl(request, arg));
}

ErrorOr<FlatPtr> Process::sys$fork(arch::Registers& registers) {
    auto* process = this->fork(registers);
    Scheduler::add_process(process);
    
    return process->id();
}

ErrorOr<FlatPtr> Process::exec(StringView path, ProcessArguments arguments) {
    auto vfs = fs::vfs();
    
    auto file = TRY(vfs->open(path, O_RDONLY, 0, m_cwd));
    auto elf = ELF(file);

    auto* process = new Process(m_id, path, false, nullptr, nullptr, m_cwd, move(arguments), m_tty);
    process->create_user_entry(elf);

    process->m_file_descriptors.resize(m_file_descriptors.size());
    for (size_t i = 0; i < m_file_descriptors.size(); i++) {
        process->m_file_descriptors[i] = m_file_descriptors[i];
    }

    process->m_parent_id = m_parent_id;

    m_id = -1; // FIXME: Actually replace the current process rather than making a new one
    Scheduler::add_process(process);

    this->kill();
    return -1;
}

ErrorOr<FlatPtr> Process::sys$execve(const char* pathname, char* const argv[], char* const envp[]) {
    ProcessArguments args;
    StringView path = this->validate_string(pathname);

    if (!argv) {
        args.argv = { path, nullptr };
    } else {
        size_t count = 0;
        while (argv[count]) {
            count++;
        }

        for (size_t i = 0; i < count; i++) {
            args.argv.append(this->validate_string(argv[i]));
        }
    }

    if (!envp) {
        args.envp = { nullptr };
    }

    return this->exec(path, args);
}

ErrorOr<FlatPtr> Process::sys$waitpid(pid_t pid, int* status, int options) {
    this->validate_write(status, sizeof(int));

    if (options & WNOHANG) {
        auto* process = Scheduler::get_process(pid);
        if (!process) {
            return Error(ECHILD);
        } else if (process->parent_id() != this->id()) {
            return Error(ECHILD);
        }

        if (process->state() == Dead) {
            *status = __WIFEXITED | process->exit_status();
            return pid;
        }

        return 0;
    }

    auto* thread = Thread::current();
    auto* blocker = WaitBlocker::create(thread, pid);

    thread->block(blocker);
    *status = blocker->status();

    return pid;
}

ErrorOr<FlatPtr> Process::sys$clock_gettime(clockid_t clock_id, timespec* ts) {
    if (clock_id < CLOCK_REALTIME || clock_id > CLOCK_MONOTONIC) {
        return Error(EINVAL);
    }

    this->validate_pointer_access(ts, sizeof(timespec), false);
    Duration time = TimeManager::query_time(clock_id);

    ts->tv_sec = time.seconds();
    ts->tv_nsec = time.nanoseconds();

    return 0;
}

ErrorOr<FlatPtr> Process::sys$clock_nanosleep(clockid_t clock_id, int flags, const timespec* req, timespec*) {
    if (clock_id < CLOCK_REALTIME || clock_id > CLOCK_MONOTONIC) {
        return Error(EINVAL);
    }

    bool is_absolute = flags & TIMER_ABSTIME;
    this->validate_read(req, sizeof(timespec));

    auto* thread = Thread::current();

    auto duration = Duration::from_timespec(*req);
    auto* blocker = new SleepBlocker(duration, clock_id, is_absolute);

    thread->block(blocker);
    return 0;
}

}
