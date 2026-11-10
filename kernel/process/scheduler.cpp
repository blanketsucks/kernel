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

static std::Atomic<pid_t> s_next_id = 0;
static bool s_initialized = false;

static SpinLock s_scheduler_lock;

static SpinLockResource<Vector<Process*>> s_processes;
static SpinLockResource<ReadyQueue> s_ready_queue {};

static Process* s_kernel_process = nullptr;

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
    for (;;) {
        asm volatile("sti; hlt");
        Scheduler::yield();
    }
}

pid_t Scheduler::generate_pid() {
    return s_next_id.fetch_add(1);
}

void Scheduler::initialize() {
    s_initialized = true;
    s_kernel_process = Process::create_kernel_process("Kernel Idle", idle);

    auto* idle_thread = s_kernel_process->get_main_thread();

    idle_thread->set_affinity(1);
    idle_thread->set_name("Idle Thread #0");
    idle_thread->set_idle_thread(true);

    auto& processor = Processor::current();
    processor.set_idle_thread(idle_thread);
}

Thread* Scheduler::create_idle_thread(int cpu) {
    auto* thread = s_kernel_process->spawn(std::format("Idle Thread #{}", cpu), idle, nullptr, false);

    thread->set_affinity(1 << cpu);
    thread->set_idle_thread(true);

    return thread;
}

void Scheduler::run() {
    auto& processor = Processor::current();
    auto* idle_thread = processor.idle_thread();

    Scheduler::lock();
    processor.initialize_context_switching(idle_thread);
}

void Scheduler::yield() {
    SpinLockGuard guard(s_scheduler_lock);

    s_processes.with([](Vector<Process*>& list) {
        for (auto& process : list) {
            process->for_each_thread([](Thread* thread) {
                if (!thread->is_blocked()) {
                    return;
                } else if (!thread->should_unblock()) {
                    return;
                }

                thread->unblock();
            });
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
        Scheduler::queue(old, Thread::Runnable);
    }

    arch::fxsave(old->fpu_state());
    arch::fxrstor(next->fpu_state());

    processor.switch_context(old, next);
}

void Scheduler::lock() {
    s_scheduler_lock.lock();
}

void Scheduler::unlock() {
    s_scheduler_lock.unlock(InterruptState::Disabled);
}

void Scheduler::add_process(Process* process) {
    s_processes.with([process](auto& list) { list.append(process); });
    process->for_each_thread([](Thread* thread) {
        Scheduler::queue(thread);
    });
}

void Scheduler::queue(Thread* thread, Optional<Thread::State> with_state) {
    if (thread->is_idle_thread()) {
        return;
    }

    s_ready_queue.with([&](ReadyQueue& queue) {
        queue.list.append(*thread);
        if (with_state.has_value()) {
            thread->set_state(with_state.value());
        }
    });
}

void Scheduler::dequeue(Thread* thread, Optional<Thread::State> with_state) {
    if (thread->is_idle_thread()) {
        return;
    }

    s_ready_queue.with([&](ReadyQueue& queue) {
        queue.list.remove(*thread);
        if (with_state.has_value()) {
            thread->set_state(with_state.value());
        }
    });
}

Thread* Scheduler::get_next_thread() {
    return s_ready_queue.with([](ReadyQueue& queue) {
        auto& processor = Processor::current();
        for (auto& thread : queue.list) {
            if (!thread.is_runnable()) {
                continue;
            } else if (thread.running_cpu() != Thread::NO_CPU) {
                continue;
            } else if (!(thread.affinity() & (1 << processor.cpu()))) {
                continue;
            }

            queue.list.remove(thread);
            thread.set_state(Thread::Running);

            thread.try_claim_cpu(processor.cpu());
            return &thread;
        }

        auto* idle_thread = processor.idle_thread();
        idle_thread->set_state(Thread::Running);

        idle_thread->try_claim_cpu(processor.cpu());
        return idle_thread;
    });
}

}
