#pragma once

#include <kernel/common.h>
#include <kernel/memory/region.h>
#include <kernel/posix/sys/types.h>
#include <kernel/process/elf.h>
#include <kernel/arch/page_directory.h>
#include <kernel/tty/tty.h>
#include <kernel/fs/vfs.h>

#include <std/hash_map.h>
#include <std/string.h>

#define VALIDATE_MEMORY_ACCESS(ptr, size, write)                            \
    auto* thread = Scheduler::current_thread();                             \
    if (thread) {                                                           \
        thread->process().validate_pointer_access(ptr, size, write);        \
    }

namespace kernel {

class Thread;

struct ProcessArguments {
    Vector<String> argv;
    Vector<String> envp;
};

class Process {
public:
    enum State {
        Alive,
        Dead
    };

    static Process* create_kernel_process(String name, void (*entry)());
    static Process* create_user_process(String name, ELF, RefPtr<fs::ResolvedInode> cwd, ProcessArguments, TTY* = nullptr);

    State state() const { return m_state; }

    pid_t id() const { return m_id; }
    pid_t parent_id() const { return m_parent_id; }

    String const& name() const { return m_name; }

    bool is_kernel() const { return m_kernel; }

    memory::RegionAllocator& region_allocator() { return m_allocator; }
    arch::PageDirectory* page_directory() const { return m_page_directory; }

    TTY* tty() const { return m_tty; }

    int exit_status() const { return m_exit_status; }

    HashMap<pid_t, Thread*>& threads() { return m_threads; }
    HashMap<pid_t, Thread*> const& threads() const { return m_threads; }

    void add_thread(Thread*);
    void remove_thread(Thread*);

    Thread* get_thread(pid_t id) const;
    Thread* get_main_thread() const;

    Thread* spawn(String name, void (*entry)());

    Process* fork(arch::Registers&);

    void kill();

    void handle_page_fault(arch::InterruptRegisters*, VirtualAddress);
    void handle_general_protection_fault(arch::InterruptRegisters*);

    FlatPtr handle_syscall(arch::Registers*);

    void* allocate(size_t size, PageFlags flags);
    void* allocate_at(VirtualAddress address, size_t size, PageFlags flags);

    void* allocate_with_physical_region(PhysicalAddress, size_t size, int prot);

    void validate_read(const void* ptr, size_t size);
    void validate_write(const void* ptr, size_t size);

    int exec(StringView path, ProcessArguments);

    void sys$exit(int status);
    int sys$open(const char* path, int flags, mode_t mode);
    int sys$close(int fd);
    ssize_t sys$read(int fd, void* buffer, size_t size);
    ssize_t sys$write(int fd, const void* buffer, size_t size);
    off_t sys$lseek(int fd, off_t offset, int whence);
    ssize_t sys$readdir(int fd, void* buffer, size_t size);
    int sys$fstat(int fd, stat* buffer);
    int sys$dup(int old_fd);
    int sys$dup2(int old_fd, int new_fd);
    void* sys$mmap(void* address, size_t size, int prot, int flags, int fd, off_t offset);
    int sys$getcwd(char* buffer, size_t size);
    int sys$chdir(const char* path);
    int sys$ioctl(int fd, unsigned request, unsigned arg);
    int sys$fork(arch::Registers&);
    int sys$execve(const char* pathname, char* const argv[], char* const envp[]);
    
private:
    friend class Scheduler;
    friend class Thread;

    void create_user_entry(ELF);

    Process(
        pid_t id, 
        String name, 
        bool kernel, 
        void (*entry)(), 
        RefPtr<fs::ResolvedInode> cwd = nullptr,
        ProcessArguments arguments = {},
        TTY* tty = nullptr
    );

    void notify_exit(Thread*);

    memory::Region* validate_pointer_access(const void* ptr, bool write);
    void validate_pointer_access(const void* ptr, size_t size, bool write);

    StringView validate_string(const char* ptr);

    RefPtr<fs::FileDescriptor> get_file_descriptor(int fd);

    State m_state = Alive;

    pid_t m_id;
    pid_t m_parent_id;

    String m_name;

    bool m_kernel = false;

    arch::PageDirectory* m_page_directory = nullptr;
    memory::RegionAllocator m_allocator;

    TTY* m_tty = nullptr;
    
    HashMap<pid_t, Thread*> m_threads;
    HashMap<pid_t, void*> m_exit_values;

    int m_exit_status = 0;

    RefPtr<fs::ResolvedInode> m_cwd;
    Vector<RefPtr<fs::FileDescriptor>> m_file_descriptors;

    ProcessArguments m_arguments;
};

};