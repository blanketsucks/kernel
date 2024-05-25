#pragma once

#include <kernel/posix/sys/types.h>
#include <kernel/process/stack.h>

#include <std/string.h>

namespace kernel {

struct ProcessArguments;
class Process;

class Blocker;

class Thread {
public:
    static constexpr u32 KERNEL_STACK_SIZE = 0x80000; // 512 KiB
    static constexpr u32 USER_STACK_SIZE = 0x100000;  // 1 MiB

    enum State : u8 {
        Running,
        Blocked,
        Sleeping,
        Zombie,
        Dead
    };

    using Entry = void (*)();

    struct Registers {
        u32 gs, fs, es, ds;
        u32 edi, esi, ebp, esp0, ebx, edx, ecx, eax;
        u32 eip, cs, eflags, esp, ss;
        u32 cr3;
    } PACKED;

    static Thread* create(u32 id, String name, Process*, Entry, ProcessArguments&);
    static Thread* create(String name, Process*, Entry, ProcessArguments&);

    u32 id() const { return m_id; }
    State state() const { return m_state; }
    String const& name() const { return m_name; }

    bool is_running() const { return m_state == Running; }
    bool is_blocked() const { return m_state == Blocked; }

    Entry entry() const { return m_entry; }

    bool is_kernel() const;
    arch::PageDirectory* page_directory() const;

    Process* process() { return m_process; }
    Registers& registers() { return m_registers; }

    Stack& kernel_stack() { return m_kernel_stack; }

    void* exit_value() const { return m_exit_value; }

    Blocker* blocker() const { return m_blocker; }

    bool should_unblock() const;
    
    void block(Blocker*);
    void unblock();

    void exit(void* value);
    void kill();

private:
    friend class Process;
    friend class Scheduler;

    Thread(String name, Process*, u32 id, Entry, ProcessArguments&);

    void create_stack();
    void setup_thread_arguments();

    void enqueue(Thread*);
    Thread* take_next();

    u32 m_id;
    State m_state;

    Entry m_entry;
    String m_name;

    Stack m_kernel_stack;
    Stack m_user_stack;

    Registers m_registers;

    Process* m_process;
    ProcessArguments& m_arguments;

    void* m_exit_value = nullptr;

    Blocker* m_blocker = nullptr;

    Thread* next = nullptr;
    Thread* prev = nullptr;
};

}