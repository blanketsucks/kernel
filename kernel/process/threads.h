#pragma once

#include <kernel/posix/sys/types.h>
#include <kernel/posix/time.h>
#include <kernel/process/stack.h>
#include <kernel/process/blocker.h>
#include <kernel/arch/registers.h>
#include <kernel/arch/cpu.h>

#include <std/format.h>
#include <std/string.h>
#include <std/time.h>
#include <std/vector.h>
#include <std/memory.h>

namespace kernel {

struct ProcessArguments;
class Process;

class Thread {
public:
    static constexpr u32 KERNEL_STACK_SIZE = 512 * KB;
    static constexpr u32 USER_STACK_SIZE = 1 * MB;

    enum State : u8 {
        Running,
        Blocked,
        Sleeping,
        Zombie,
        Dead
    };

    using Entry = void (*)(void*);

    static Thread* create(u32 id, String name, Process*, Entry, void* entry_data, ProcessArguments&);
    static Thread* create(String name, Process*, Entry, void* entry_data, ProcessArguments&);

    static Thread* current();

    pid_t id() const { return m_id; }
    pid_t pid() const;

    State state() const { return m_state; }
    String const& name() const { return m_name; }

    bool is_running() const { return m_state == Running; }
    bool is_blocked() const { return m_state == Blocked; }

    Entry entry() const { return m_entry; }

    bool is_kernel() const;
    arch::PageDirectory* page_directory() const;

    arch::FPUState& fpu_state() const { return *m_fpu_state; }

    Process* process() { return m_process; }
    arch::ThreadRegisters& registers() { return m_registers; }

    Stack& kernel_stack() { return m_kernel_stack; }
    Stack& user_stack() { return m_user_stack; }

    void* exit_value() const { return m_exit_value; }

    RefPtr<Blocker> const& blocker() const { return m_blocker; }

    bool should_unblock() const;
    
    void set_blocker(RefPtr<Blocker> blocker) { m_blocker = blocker; }

    void sleep(clockid_t, const Duration& duration);
    void sleep(const Duration& duration) {
        this->sleep(CLOCK_MONOTONIC, duration);
    }

    void block(RefPtr<Blocker>);
    void unblock();

    void exit(void* value);
    void kill();

private:
    friend class Process;
    friend class Scheduler;

    Thread(String name, Process*, pid_t id, Entry, void* entry_data, ProcessArguments&);
    Thread(Process*, arch::Registers*);

    void create_stack();
    void set_initial_stack_state(FlatPtr sp, arch::ThreadRegisters&);
    
    void setup_thread_arguments();
    void prepare_argument_vector(Vector<String> const& src, Vector<FlatPtr>& dst);

    void enqueue(Thread*);
    Thread* take_next();

    pid_t m_id;
    State m_state;

    Entry m_entry;
    void* m_entry_data = nullptr;

    String m_name;

    Stack m_kernel_stack;
    Stack m_user_stack;

    arch::ThreadRegisters m_registers;

    Process* m_process;
    ProcessArguments& m_arguments;

    u8* m_arguments_region = nullptr;
    size_t m_arguments_offset = 0;
    
    void* m_exit_value = nullptr;

    mutable RefPtr<Blocker> m_blocker = nullptr;

    arch::FPUState* m_fpu_state = nullptr;

    Thread* next = nullptr;
    Thread* prev = nullptr;
};

}