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
    return new Process(Scheduler::generate_pid(), move(name), true, entry, data);
}

ErrorOr<Process*> Process::create_user_process(String path, RefPtr<fs::ResolvedInode> cwd, TTY* tty) {
    auto vfs = fs::vfs();
    auto file = TRY(vfs->open(path, O_RDONLY, 0, cwd));

    ELF elf(file);

    ProcessArguments arguments;
    arguments.argv = { path };

    auto* process = new Process(Scheduler::generate_pid(), move(path), false, nullptr, nullptr, cwd, move(arguments), tty);
    TRY(process->create_user_entry(elf));

    return process;
}

ErrorOr<Process*> Process::create_user_process(String name, ELF elf, RefPtr<fs::ResolvedInode> cwd, ProcessArguments arguments, TTY* tty) {
    auto* process = new Process(Scheduler::generate_pid(), move(name), false, nullptr, nullptr, cwd, move(arguments), tty);
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
    TTY* tty
) : m_state(Alive), m_id(id), m_name(move(name)), m_kernel(kernel), m_tty(tty), m_arguments(move(arguments)) {
    if (!cwd) {
        m_cwd = fs::vfs()->root();
    } else {
        m_cwd = move(cwd);
    }

    if (!kernel) {
        m_page_directory = arch::PageDirectory::create_user_page_directory();
        m_allocator = memory::RegionAllocator::create(
            { VirtualAddress { PAGE_SIZE }, static_cast<size_t>(g_boot_info->kernel_virtual_base - PAGE_SIZE) },
            m_page_directory
        );
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

Process::Process(
    String name, Process* parent
) : m_id(Scheduler::generate_pid()), m_parent_id(parent->id()), m_name(move(name)), m_kernel(false), m_cwd(parent->m_cwd) {
    m_page_directory = arch::PageDirectory::create_user_page_directory();
    m_allocator = parent->m_allocator->clone(m_page_directory);

    m_file_descriptors.resize(parent->m_file_descriptors.size());
    for (size_t i = 0; i < parent->m_file_descriptors.size(); i++) {
        m_file_descriptors[i] = parent->m_file_descriptors[i];
    }
}

ErrorOr<void> Process::create_user_entry(ELF elf) {
    TRY(elf.load());

    if (elf.has_interpreter()) {
        String interpreter = elf.interpreter();

        auto* vfs = fs::vfs();
        auto file = TRY(vfs->open(interpreter, O_RDONLY, 0, m_cwd));

        elf = ELF(file);
        TRY(elf.load());

        Vector<String> argv = m_arguments.argv;

        m_arguments.argv = { interpreter };
        m_arguments.argv.extend(move(argv));
    }

    auto& file = elf.file();
    for (auto& ph : elf.program_headers()) {
        if (ph.p_type != PT_LOAD || ph.p_vaddr == 0) {
            continue;
        }

        VirtualAddress address { std::align_down(ph.p_vaddr, PAGE_SIZE) };
        size_t size = std::align_up(ph.p_memsz, PAGE_SIZE);

        u8* region = reinterpret_cast<u8*>(TRY(this->allocate_at(address, size, PageFlags::Write)));
        TemporaryMapping temp(*m_page_directory, region, size);

        file.seek(ph.p_offset, SEEK_SET);
        TRY(file.read(temp.ptr() + (ph.p_vaddr - address), ph.p_filesz));
    }

    auto entry = reinterpret_cast<void(*)(void*)>(elf.entry());
    
    auto* thread = Thread::create(m_id, "main", this, entry, nullptr, m_arguments);
    this->add_thread(thread);

    return {};
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
    auto thread = Thread::create(Scheduler::generate_pid(), move(name), this, entry, data, m_arguments);
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
        m_page_directory->map(region->offset_by(i), pa, pflags);
    }

    region->set_kernel_managed(true);
    return region->base().to_ptr();
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
        m_page_directory->map(region->offset_by(i), address + i, pflags);
    }

    region->set_kernel_managed(true);
    return region->base().to_ptr();
}

ErrorOr<void*> Process::allocate_file_backed_region(fs::File* file, size_t size) {
    ASSERT(size % PAGE_SIZE == 0, "size must be page aligned");
    auto* region = m_allocator->create_file_backed_region(file, size);
    if (!region) {
        return Error(ENOMEM);
    }

    return region->base().to_ptr();
}

void Process::notify_exit(Thread* thread) {
    this->remove_thread(thread);
}

void Process::handle_page_fault(arch::InterruptRegisters* regs, VirtualAddress address) {
    // TODO: Send proper signal once that is implemented
    PageFault fault = regs->errno;
    bool is_null_pointer_dereference = address < PAGE_SIZE;

    auto* region = m_allocator->find_region(address, true);
    if (region && fault.rw && fault.present) {
        arch::PageTableEntry* entry = m_page_directory->get_page_table_entry(address);
        PhysicalAddress pa = entry->physical_address();

        auto* page = MM->get_physical_page(pa);

        if (!page) {
            goto unrecoverable_fault;
        }
        
        bool is_cow = page->flags & PhysicalPage::CoW;
        if (!is_cow) {
            goto unrecoverable_fault;
        }

        if (page->ref_count > 1) {
            page->ref_count--;
        } else {
            page->flags &= ~PhysicalPage::CoW;
        }

        void* frame = MUST(MM->allocate_page_frame());

        // TODO: Move this to a function in MemoryManager
        page = MM->get_physical_page(reinterpret_cast<PhysicalAddress>(frame));
        page->ref_count++;

        MM->copy_physical_memory(frame, reinterpret_cast<void*>(pa), PAGE_SIZE);
        entry->set_physical_address(reinterpret_cast<PhysicalAddress>(frame));

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

        StringView message = MemoryManager::get_fault_message(fault, region);
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

    auto* file = region->file();
    size_t offset = std::align_down(region->offset_in(address), PAGE_SIZE);

    void* frame = MUST(MM->allocate_page_frame());
    m_page_directory->map(region->offset_by(offset), reinterpret_cast<PhysicalAddress>(frame), PageFlags::Write | PageFlags::User);

    size_t size = std::min(file->size() - offset, PAGE_SIZE);
    auto result = file->read(region->offset_by(offset).to_ptr(), size, region->offset() + offset);

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
    auto* region = m_allocator->find_region(VirtualAddress { ptr }, true);

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

    size_t offset = region->offset_in(VirtualAddress { ptr });
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
    size_t offset = region->offset_in(VirtualAddress { ptr });

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

StringView Process::validate_string(const char* ptr, size_t length) {
    this->validate_pointer_access(ptr, length, false);
    return { ptr, length };
}

RefPtr<fs::FileDescriptor> Process::get_file_descriptor(int fd) {
    if (fd < 0 || static_cast<size_t>(fd) >= m_file_descriptors.size()) {
        return nullptr;
    }

    return m_file_descriptors[fd];
}

void Process::kill() {
    for (auto& fd : m_file_descriptors) {
        fd.~RefPtr();
    }

    if (!m_allocator) {
        m_state = Zombie;
        for (auto& [_, thread] : m_threads) {
            thread->kill();
        }

        Scheduler::yield();

        __builtin_unreachable();
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
    
    m_state = Zombie;
    for (auto& [_, thread] : m_threads) {
        thread->kill();
    }

    Scheduler::yield();
}

}
