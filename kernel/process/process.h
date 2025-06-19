#pragma once

#include <kernel/common.h>
#include <kernel/memory/region.h>
#include <kernel/posix/sys/types.h>
#include <kernel/process/elf.h>
#include <kernel/arch/page_directory.h>
#include <kernel/tty/tty.h>
#include <kernel/fs/vfs.h>
#include <kernel/sync/resource.h>
#include <kernel/arch/registers.h>

#include <std/hash_map.h>
#include <std/string.h>

namespace kernel {

class Thread;

struct ProcessArguments {
    Vector<String> argv;
    Vector<String> envp;
};

class Process {
public:
    enum State {
        Alive = 1,
        Dead
    };

    template<typename F>
    static void kernel_process_entry(void* data) {
        F* entry = reinterpret_cast<F*>(data);
        (*entry)();
        delete entry;
    }

    static void kernel_process_entry(void* data) {
        auto* entry = reinterpret_cast<void(*)()>(data);
        (*entry)();
    }

    template<typename F>
    static Process* create_kernel_process(String name, F entry) {
        F* data = new F(move(entry));
        return create_kernel_process(move(name), kernel_process_entry<F>, data);
    }

    static Process* create_kernel_process(String name, void (*entry)()) {
        return create_kernel_process(move(name), kernel_process_entry, reinterpret_cast<void*>(entry));
    }
    
    static Process* create_kernel_process(String name, void (*entry)(void*), void* data = nullptr);

    static ErrorOr<Process*> create_user_process(String path, RefPtr<fs::ResolvedInode> cwd, TTY* tty = nullptr);
    static ErrorOr<Process*> create_user_process(String name, ELF, RefPtr<fs::ResolvedInode> cwd, ProcessArguments, TTY* tty = nullptr);

    static Process* current();

    State state() const { return m_state; }

    pid_t id() const { return m_id; }
    pid_t parent_id() const { return m_parent_id; }

    String const& name() const { return m_name; }

    bool is_kernel() const { return m_kernel; }

    arch::PageDirectory* page_directory() const { return m_page_directory; }

    TTY* tty() const { return m_tty; }

    int exit_status() const { return m_exit_status; }

    HashMap<pid_t, Thread*>& threads() { return m_threads; }
    HashMap<pid_t, Thread*> const& threads() const { return m_threads; }

    void add_thread(Thread*);
    void remove_thread(Thread*);

    Thread* get_thread(pid_t id) const;
    Thread* get_main_thread() const;

    Thread* spawn(String name, void (*entry)(void*), void* data = nullptr);

    Process* fork(arch::Registers&);

    void kill();

    void handle_page_fault(arch::InterruptRegisters*, VirtualAddress);
    void handle_general_protection_fault(arch::InterruptRegisters*);

    FlatPtr handle_syscall(arch::Registers*);

    void* allocate(size_t size, PageFlags flags);
    void* allocate_at(VirtualAddress address, size_t size, PageFlags flags);

    void* allocate_with_physical_region(PhysicalAddress, size_t size, int prot);
    void* allocate_file_backed_region(fs::File* file, size_t size);

    void validate_read(const void* ptr, size_t size);
    void validate_write(const void* ptr, size_t size);

    ErrorOr<FlatPtr> exec(StringView path, ProcessArguments);

    [[noreturn]] void sys$exit(int status);

    ErrorOr<FlatPtr> sys$getpid();
    ErrorOr<FlatPtr> sys$getppid();
    ErrorOr<FlatPtr> sys$gettid();

    ErrorOr<FlatPtr> sys$open(const char* path, int flags, mode_t mode);
    ErrorOr<FlatPtr> sys$close(int fd);
    ErrorOr<FlatPtr> sys$read(int fd, void* buffer, size_t size);
    ErrorOr<FlatPtr> sys$write(int fd, const void* buffer, size_t size);
    ErrorOr<FlatPtr> sys$lseek(int fd, off_t offset, int whence);
    ErrorOr<FlatPtr> sys$readdir(int fd, void* buffer, size_t size);
    ErrorOr<FlatPtr> sys$fstat(int fd, stat* buffer);
    ErrorOr<FlatPtr> sys$dup(int old_fd);
    ErrorOr<FlatPtr> sys$dup2(int old_fd, int new_fd);
    ErrorOr<FlatPtr> sys$ioctl(int fd, unsigned request, unsigned arg);

    ErrorOr<FlatPtr> sys$mmap(mmap_args*);

    ErrorOr<FlatPtr> sys$getcwd(char* buffer, size_t size);
    ErrorOr<FlatPtr> sys$chdir(const char* path);
    
    ErrorOr<FlatPtr> sys$fork(arch::Registers&);
    ErrorOr<FlatPtr> sys$execve(const char* pathname, char* const argv[], char* const envp[]);
    
    ErrorOr<FlatPtr> sys$waitpid(pid_t pid, int* status, int options);

    ErrorOr<FlatPtr> sys$clock_gettime(clockid_t clock_id, timespec* ts);
    ErrorOr<FlatPtr> sys$clock_nanosleep(clockid_t clock_id, int flags, const timespec* req, timespec* rem);
    
private:
    friend class Scheduler;
    friend class Thread;

    ErrorOr<void> create_user_entry(ELF);

    Process(
        pid_t id, 
        String name, 
        bool kernel, 
        void (*entry)(void*),
        void* entry_data = nullptr,
        RefPtr<fs::ResolvedInode> cwd = nullptr,
        ProcessArguments arguments = {},
        TTY* tty = nullptr,
        Process* parent = nullptr
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
    RefPtr<memory::RegionAllocator> m_allocator;

    TTY* m_tty = nullptr;
    
    HashMap<pid_t, Thread*> m_threads;
    HashMap<pid_t, void*> m_exit_values;

    int m_exit_status = 0;

    RefPtr<fs::ResolvedInode> m_cwd;
    Vector<RefPtr<fs::FileDescriptor>> m_file_descriptors;

    ProcessArguments m_arguments;
};

};