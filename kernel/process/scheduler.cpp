#include <kernel/process/scheduler.h>
#include <kernel/process/threads.h>
#include <kernel/process/process.h>

namespace kernel {

extern "C" void _switch_context(Thread::Registers*, Thread::Registers*);
extern "C" void _switch_context_no_state(Thread::Registers*);

static u32 s_next_id = 1;

static Vector<Process*> s_processes;

static Thread* s_next_thread = nullptr;
static Thread* s_current_thread = nullptr;

static Process* s_kernel_process = nullptr;

static arch::TSS s_tss;

void _idle() {
    Scheduler::yield();
    while (true) {
        asm volatile("hlt");
    }
}

u32 generate_id() {
    return s_next_id++;
}

void Scheduler::init() {
    memset(&s_tss, 0, sizeof(arch::TSS));

	s_tss.ss0 = 0x10;
	s_tss.cs = 0x0b;
	s_tss.ss = 0x13;
	s_tss.ds = 0x13;
	s_tss.es = 0x13;
	s_tss.fs = 0x13;
	s_tss.gs = 0x13;

    s_kernel_process = Process::create_kernel_process("Kernel Idle", _idle);
    auto thread = s_kernel_process->get_main_thread();

    s_current_thread = thread;
    _switch_context_no_state(&thread->m_registers);
}

void Scheduler::yield() {
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

    auto& kernel_stack = next->kernel_stack();
    s_tss.esp0 = kernel_stack.top();

    Thread* old = s_current_thread;
    s_current_thread = next;

    _switch_context(&old->m_registers, &next->m_registers);
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
    return s_current_thread->process();
}

void Scheduler::set_current_thread(Thread* thread) {
    s_current_thread = thread;
}

arch::TSS& Scheduler::tss() {
    return s_tss;
}

}