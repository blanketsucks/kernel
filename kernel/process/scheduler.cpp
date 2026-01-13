#include <kernel/process/scheduler.h>
#include <kernel/process/threads.h>
#include <kernel/process/process.h>

#include <kernel/arch/cpu.h>
#include <kernel/arch/processor.h>
#include <kernel/arch/interrupts.h>

#include <std/format.h>

namespace kernel {

static u32 s_next_id = 0;
static bool s_initialized = false;

static Vector<Process*> s_processes;

static Thread* s_next_thread = nullptr;
static Thread* s_current_thread = nullptr;

static Process* s_kernel_process = nullptr;

static bool s_invoked_async = false;

void Scheduler::invoke_async() {
    s_invoked_async = true;
}

bool Scheduler::is_invoked_async() {
    return s_invoked_async;
}

Process* Scheduler::get_process(pid_t id) {
    for (auto& process : s_processes) {
        if (process->id() == id) {
            return process;
        }
    }
    
    return nullptr;
}

void _idle() {
    Scheduler::yield();
    while (true) {
        asm volatile("hlt");
        Scheduler::yield();
    }
}

pid_t Scheduler::generate_pid() {
    return s_next_id++;
}

Vector<Process*>& Scheduler::processes() {
    return s_processes;
}

void Scheduler::init() {
    s_initialized = true;
    s_kernel_process = Process::create_kernel_process("Kernel Idle", _idle);
    
    auto thread = s_kernel_process->get_main_thread();
    s_current_thread = thread;
    
    auto& processor = Processor::instance();
    processor.initialize_context_switching(thread);
}

void Scheduler::yield(bool if_idle) {
    if (!s_initialized || !s_current_thread) {
        return;
    }

    arch::InterruptDisabler disabler;
    s_invoked_async = false;

    for (auto& process : s_processes) {
        for (auto& [_, thread] : process->threads()) {
            if (thread->is_blocked() && thread->should_unblock()) {
                thread->unblock();
            }
        }
    }

    Thread* next = Scheduler::get_next_thread();
    if (!next || next == s_current_thread) {
        return;
    }
    
    Thread* old = s_current_thread;
    if (if_idle && old->process() != s_kernel_process) {
        return;
    }

    s_current_thread = next;
    if (old->is_running() && old->pid() != s_kernel_process->id()) {
        Scheduler::queue(old);
    }

    arch::fxsave(old->fpu_state());

    auto& processor = Processor::instance();
    processor.switch_context(old, next);

    arch::fxrstor(next->fpu_state());
}

void Scheduler::add_process(Process* process) {
    s_processes.append(process);
    for (auto& [_, thread] : process->threads()) {
        Scheduler::queue(thread);
    }
}

void Scheduler::queue(Thread* thread) {
    if (!s_next_thread) {
        s_next_thread = thread;
    } else {
        s_next_thread->enqueue(thread);
    }
}

Thread* Scheduler::get_next_thread() {
    while (s_next_thread && !s_next_thread->is_running()) {
        s_next_thread = s_next_thread->take_next();
    }

    if (!s_next_thread) {
        if (s_current_thread->is_running()) {
            return s_current_thread;
        }
        
        return s_kernel_process->get_main_thread();
    }

    Thread* thread = s_next_thread;
    s_next_thread = s_next_thread->take_next();

    return thread;
}

Thread* Scheduler::next_thread() {
    return s_next_thread;
}

Thread* Scheduler::set_next_thread(Thread* thread) {
    return s_next_thread = thread;
}

Thread* Scheduler::current_thread() {
    return s_current_thread;
}

Process* Scheduler::current_process() {
    if (!s_current_thread) {
        return nullptr;
    }

    return s_current_thread->process();
}

void Scheduler::set_current_thread(Thread* thread) {
    s_current_thread = thread;
}

}
