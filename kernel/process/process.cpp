#include "kernel/fs/inode.h"
#include "std/memory.h"
#include <kernel/process/process.h>
#include <kernel/process/threads.h>
#include <kernel/process/scheduler.h>

#include <kernel/posix/unistd.h>

#include <std/format.h>

namespace kernel {

Process* Process::create_kernel_process(String name, void (*entry)()) {
    return new Process(1, move(name), true, entry);
}

Process* Process::create_user_process(String name, ELF& elf, RefPtr<fs::ResolvedInode> cwd) {
    auto* process = new Process(generate_id(), move(name), false, nullptr, cwd);
    process->create_user_entry(elf);

    return process;
}

Process::Process(
    pid_t id, String name, bool kernel, void (*entry)(), RefPtr<fs::ResolvedInode> cwd
) : m_id(id), m_name(move(name)), m_kernel(kernel), m_cwd(cwd) {
    if (!kernel) {
        m_page_directory = arch::PageDirectory::create_user_page_directory();
        m_memory_region = memory::Region(PAGE_SIZE, KERNEL_VIRTUAL_BASE - PAGE_SIZE, m_page_directory);
    } else {
        m_page_directory = arch::PageDirectory::kernel_page_directory();
        
        auto thread = Thread::create(m_id, "main", this, entry);
        this->add_thread(thread);
    }
}

void Process::create_user_entry(ELF& elf) {
    auto& file = elf.file();
    elf.load();

    m_page_directory->switch_to(); // Should we do this?
    for (auto& ph : elf.program_headers()) {
        if (ph.p_type != PT_LOAD || ph.p_vaddr == 0) {
            continue;
        }

        uintptr_t address = std::align_down(ph.p_vaddr, static_cast<size_t>(PAGE_SIZE));
        size_t size = std::align_up(ph.p_memsz, static_cast<size_t>(PAGE_SIZE));
        
        u8* region = reinterpret_cast<u8*>(this->allocate_at(address, size, PageFlags::Write));

        file.seek(ph.p_offset, SEEK_SET);
        file.read(region + (ph.p_vaddr - address), ph.p_filesz);
    }

    auto entry = reinterpret_cast<void(*)()>(elf.entry());

    auto* dir = arch::PageDirectory::kernel_page_directory();
    dir->switch_to();
    
    auto* thread = Thread::create(m_id, "main", this, entry);
    this->add_thread(thread);
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
    auto thread = Thread::create(generate_id(), move(name), this, entry);
    this->add_thread(thread);

    Scheduler::queue(thread);
    return thread;
}

void* Process::allocate(size_t size, PageFlags flags) {
    if (this->is_kernel()) {
        return MM->allocate_kernel_region(size);
    }

    return MM->allocate(m_memory_region, size, flags | PageFlags::User);
}

void* Process::allocate_at(uintptr_t address, size_t size, PageFlags flags) {
    if (this->is_kernel()) {
        return MM->allocate_kernel_region(size);
    }

    return MM->allocate_at(m_memory_region, address, size, flags | PageFlags::User);
}

void Process::notify_exit(Thread* thread) {
    this->remove_thread(thread);
}

void Process::validate_pointer_access(const void* ptr, size_t size, bool write) {
    // FIXME: The ptr + size may be split between two memory regions.
    auto* space = m_memory_region.find_space_containing(reinterpret_cast<uintptr_t>(ptr));
    if (!space) {
        serial::printf("[%s (PID %u)] Invalid memory access at 0x%p. Killing.\n", m_name.data(), m_id, ptr);
        this->kill();
    } else if (write && !space->is_writable()) {
        serial::printf("[%s (PID %u)] Write to read-only memory at 0x%p. Killing.\n", m_name.data(), m_id, ptr);
        this->kill();
    } 
    
    size_t offset = reinterpret_cast<uintptr_t>(ptr) - space->address();
    if (offset + size > space->size()) {
        serial::printf("[%s (PID %u)] Invalid memory access at 0x%p. Killing.\n", m_name.data(), m_id, ptr);
        this->kill();
    }
}

void Process::validate_read(const void* ptr, size_t size) {
    this->validate_pointer_access(ptr, size, false);
}

void Process::validate_write(const void* ptr, size_t size) {
    this->validate_pointer_access(ptr, size, true);
}

void Process::kill() {
    m_state = Dead;
    for (auto& [_, thread] : m_threads) {
        thread->kill();
    }

    Scheduler::yield();
}

void Process::sys$exit(int status) {
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
    if (fd < 0 || static_cast<size_t>(fd) >= m_file_descriptors.size()) {
        return -EBADF;
    }

    this->validate_read(buffer, size);
    auto& file = m_file_descriptors[fd];
    if (!file) {
        return -EBADF;
    }

    return file->read(buffer, size);
}

ssize_t Process::sys$write(int fd, const void* buffer, size_t size) {
    if (fd < 0 || static_cast<size_t>(fd) >= m_file_descriptors.size()) {
        return -EBADF;
    }

    this->validate_write(buffer, size);
    auto& file = m_file_descriptors[fd];
    if (!file) {
        return -EBADF;
    }

    return file->write(buffer, size);
}

}