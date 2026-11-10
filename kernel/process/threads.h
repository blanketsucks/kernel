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
#include <std/atomic.h>
#include <std/memory.h>
#include <std/intrusive_list.h>

namespace kernel {

struct ProcessArguments;
class Process;

class Thread {
public:
    static constexpr u32 KERNEL_STACK_SIZE = 512 * KiB;
    static constexpr u32 USER_STACK_SIZE   = 1 * MiB;

    static constexpr u32 DEFAULT_AFFINITY = 0xffffffff;

    static constexpr u32 NO_CPU = 0xffffffff;

    enum State : u8 {
        Runnable,
        Running,
        Blocked,
        Sleeping,
        Zombie,
        Dead
    };

    using Entry = void (*)(void*);

    static Thread* create(pid_t id, String name, Process*, Entry, void* entry_data, ProcessArguments&);
    static Thread* create(String name, Process*, Entry, void* entry_data, ProcessArguments&);

    static Thread* current();

    pid_t id() const { return m_id; }
    pid_t pid() const;

    u32 affinity() const { return m_affinity; }
    void set_affinity(u32 affinity) { m_affinity = affinity; }

    bool is_main_thread() const;

    State state() const { return m_state; }
    void set_state(State state) { m_state = state; }

    String const& name() const { return m_name; }
    void set_name(String name) { m_name = move(name); }

    bool is_running() const { return m_state == Running; }
    bool is_runnable() const { return m_state == Runnable; }
    bool is_blocked() const { return m_state == Blocked; }

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

    bool try_claim_cpu(u32 cpu) {
        u32 expected = NO_CPU;
        return m_running_cpu.compare_exchange_strong(expected, cpu);
    }

    void release_cpu() { m_running_cpu.store(NO_CPU); }
    u32 running_cpu() { return m_running_cpu.load(); }

    bool is_idle_thread() const { return m_idle; }
    void set_idle_thread(bool value) { m_idle = value; }

private:
    friend class Process;
    friend class Scheduler;
    friend class Processor;

    friend struct ReadyQueue;

    Thread(String name, Process*, pid_t id, Entry, void* entry_data, ProcessArguments&);
    Thread(Process*, arch::Registers*);

    void init_exec_state(FlatPtr ip, bool new_kernel_stack = true);
    void set_initial_stack_state(FlatPtr sp);

    void setup_thread_arguments();
    void prepare_argument_vector(Vector<String> const& src, Vector<FlatPtr>& dst);

    void reset_arguments_region();

    pid_t m_id;
    State m_state;

    u32 m_affinity = DEFAULT_AFFINITY;

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

    std::ListNode m_ready_node;

    std::Atomic<u32> m_running_cpu { NO_CPU };

    bool m_idle = false;
};

}
