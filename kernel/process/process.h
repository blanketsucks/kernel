#pragma once

#include <kernel/common.h>
#include <kernel/memory/region.h>
#include <kernel/posix/sys/types.h>
#include <kernel/arch/paging.h>
#include <kernel/fs/vfs.h>
#include <kernel/elf.h>

#include <std/hash_map.h>
#include <std/string.h>

namespace kernel {

class Thread;

class Process {
public:
    enum State {
        Alive,
        Dead
    };

    static Process* create_kernel_process(String name, void (*entry)());
    static Process* create_user_process(String name, ELF&, RefPtr<fs::ResolvedInode> cwd);

    State state() const { return m_state; }

    pid_t id() const { return m_id; }
    String const& name() const { return m_name; }

    bool is_kernel() const { return m_kernel; }
    arch::PageDirectory* page_directory() const { return m_page_directory; }

    int exit_status() const { return m_exit_status; }

    HashMap<u32, Thread*>& threads() { return m_threads; }
    HashMap<u32, Thread*> const& threads() const { return m_threads; }

    void add_thread(Thread*);
    void remove_thread(Thread*);

    Thread* get_thread(u32 id) const;
    Thread* get_main_thread() const;

    Thread* spawn(String name, void (*entry)());

    Process* fork();

    void kill();

    void sys$exit(int status);

    int sys$open(const char* path, size_t path_length, int flags, mode_t mode);
    int sys$close(int fd);
    ssize_t sys$read(int fd, void* buffer, size_t size);
    ssize_t sys$write(int fd, const void* buffer, size_t size);

private:
    friend class Scheduler;
    friend class Thread;

    void create_user_entry(ELF&);

    void* allocate(size_t size, PageFlags flags);
    void* allocate_at(uintptr_t address, size_t size, PageFlags flags);

    Process(pid_t id, String name, bool kernel, void (*entry)(), RefPtr<fs::ResolvedInode> cwd = nullptr);

    void notify_exit(Thread*);

    void validate_pointer_access(const void* ptr, size_t size, bool write);

    void validate_read(const void* ptr, size_t size);
    void validate_write(const void* ptr, size_t size);

    State m_state = Alive;

    pid_t m_id;
    pid_t m_parent_id;

    String m_name;

    bool m_kernel = false;

    arch::PageDirectory* m_page_directory = nullptr;
    memory::Region m_memory_region;
    
    HashMap<u32, Thread*> m_threads;
    HashMap<u32, void*> m_exit_values;

    int m_exit_status = 0;

    RefPtr<fs::ResolvedInode> m_cwd;
    Vector<RefPtr<fs::FileDescriptor>> m_file_descriptors;
};

};