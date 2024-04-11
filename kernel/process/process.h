#pragma once

#include <kernel/common.h>
#include <kernel/memory/paging.h>
#include <kernel/posix/sys/types.h>

#include <std/hash_map.h>
#include <std/string.h>

namespace kernel {

class Thread;

class Process {
public:
    enum State {
        Running,
        Stopped,
        Zombie,
        Dead
    };

    static Process* create_kernel_process(String name, void (*entry)());

    pid_t id() const { return m_id; }
    String const& name() const { return m_name; }

    bool is_kernel() const { return m_kernel; }
    memory::PageDirectory* page_directory() const { return m_page_directory; }

    HashMap<u32, Thread*>& threads() { return m_threads; }
    HashMap<u32, Thread*> const& threads() const { return m_threads; }

    void add_thread(Thread*);
    void remove_thread(Thread*);

    Thread* get_thread(u32 id) const;
    Thread* get_main_thread() const;

    Thread* spawn(String name, void (*entry)());

private:
    friend class Scheduler;
    friend class Thread;

    Process(pid_t id, String name, bool kernel, void (*entry)());

    void notify_exit(Thread*);

    pid_t m_id;
    String m_name;

    bool m_kernel = false;
    memory::PageDirectory* m_page_directory = nullptr;
    
    HashMap<u32, Thread*> m_threads;
    HashMap<u32, void*> m_exit_values;
};

};