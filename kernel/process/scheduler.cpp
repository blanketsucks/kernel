#include <kernel/process/scheduler.h>
#include <kernel/process/threads.h>
#include <kernel/process/process.h>
#include <kernel/sync/resource.h>
#include <kernel/arch/cpu.h>
#include <kernel/arch/processor.h>
#include <kernel/arch/interrupts.h>

#include <std/format.h>

namespace kernel {

struct ReadyQueue {
    std::IntrusiveList<Thread, __builtin_offsetof(Thread, m_ready_node)> list;
};

static u32 s_next_id = 0;
static bool s_initialized = false;

static SpinLock s_scheduler_lock;

static SpinLockResource<Vector<Process*>> s_processes;

static Process* s_kernel_process = nullptr;

static SpinLockResource<ReadyQueue> s_ready_queue {};

static bool s_invoked_async = false;

void Scheduler::invoke_async() {
    s_invoked_async = true;
}

bool Scheduler::is_invoked_async() {
    return s_invoked_async;
}

Process* Scheduler::get_process(pid_t id) {
    return s_processes.with([id](auto& list) -> Process* {
        for (auto& process : list) {
            if (process->id() == id) {
                return process;
            }
        }

        return nullptr;
    });
}

void idle(void*) {
    Scheduler::yield();

    while (true) {
        asm volatile("hlt");
        Scheduler::yield();
    }
}

pid_t Scheduler::generate_pid() {
    return s_next_id++;
}

void Scheduler::initialize() {
    s_initialized = true;
    s_kernel_process = Process::create_kernel_process("Kernel Idle", idle);

    auto* idle_thread = s_kernel_process->get_main_thread();

    idle_thread->set_affinity(1);
    idle_thread->set_name("Idle Thread #0");

    auto& processor = Processor::current();
    processor.set_idle_thread(idle_thread);
}

Thread* Scheduler::create_idle_thread(int cpu) {
    auto* thread = s_kernel_process->spawn(std::format("Idle Thread #{}", cpu), idle);
    thread->set_affinity(1 << cpu);

    return thread;
}

void Scheduler::run() {
    auto& processor = Processor::current();

    auto* idle_thread = processor.idle_thread();
    idle_thread->set_state(Thread::Running);

    processor.initialize_context_switching(idle_thread);
}

void Scheduler::yield() {
    arch::InterruptDisabler disabler;

    s_invoked_async = false;

    s_processes.with([](Vector<Process*>& list) {
        for (auto& process : list) {
            for (auto& [_, thread] : process->threads()) {
                if (thread->is_blocked() && thread->should_unblock()) {
                    thread->unblock();
                }
            }
        }
    });

    auto& processor = Processor::current();
    if (!processor.current_thread()) {
        return;
    }

    Thread* next = Scheduler::get_next_thread();
    if (next == processor.current_thread()) {
        return;
    }

    Thread* old = processor.current_thread();
    if (old->is_running()) {
        old->set_state(Thread::Runnable);
        Scheduler::queue(old);
    }

    arch::fxsave(old->fpu_state());

    processor.switch_context(old, next);

    arch::fxrstor(next->fpu_state());
}

void Scheduler::add_process(Process* process) {
    s_processes.with([process](auto& list) { list.append(process); });
    for (auto& [_, thread] : process->threads()) {
        Scheduler::queue(thread);
    }
}

void Scheduler::queue(Thread* thread) {
    s_ready_queue.with([&](ReadyQueue& queue) {
        queue.list.append(*thread);
    });
}

void Scheduler::dequeue(Thread* thread) {
    s_ready_queue.with([&](ReadyQueue& queue) {
        queue.list.remove(*thread);
    });
}

Thread* Scheduler::get_next_thread() {
    return s_ready_queue.with([](ReadyQueue& queue) {
        auto& processor = Processor::current();
        for (auto& thread : queue.list) {
            if (thread.is_running() || thread.is_blocked()) {
                continue;
            }

            if (!(thread.affinity() & (1 << processor.cpu()))) {
                continue;
            }

            queue.list.remove(thread);
            thread.set_state(Thread::Running);

            return &thread;
        }

        return processor.idle_thread();
    });
}

}
