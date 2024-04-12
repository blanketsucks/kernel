#include <kernel/process/threads.h>
#include <kernel/process/scheduler.h>
#include <kernel/process/process.h>
#include <kernel/process/blocker.h>

namespace kernel {

extern "C" void _first_yield();

Thread* Thread::create(u32 id, String name, Process* process, Entry entry) {
    return new Thread(move(name), process, id, entry);
}

Thread* Thread::create(String name, Process* process, Entry entry) {
    return new Thread(move(name), process, generate_id(), entry);
}

Thread::Thread(
    String name, Process* process, u32 id, Entry entry
) : m_id(id), m_state(Running), m_entry(entry), m_name(move(name)), m_process(process) {
    this->create_stack();
}

bool Thread::is_kernel() const {
    return m_process->is_kernel();
}

memory::PageDirectory* Thread::page_directory() const {
    return m_process->page_directory();
}

void Thread::create_stack() {
    void* kernel_stack = MM->allocate(KERNEL_STACK_SIZE);
    m_kernel_stack = Stack(kernel_stack, KERNEL_STACK_SIZE);

    u8 selector = 0x08; // Data segment selector
    u8 segment = 0x10;  // Code segment selector

    if (!this->is_kernel()) {
        void* user_stack = MM->allocate(USER_STACK_SIZE);
        m_user_stack = Stack(user_stack, USER_STACK_SIZE);

        selector = 0x18;
        segment = 0x20;
    } else {
        m_user_stack = m_kernel_stack;
    }

    u32 eflags = 0;
    asm volatile("pushf; pop %0" : "=r"(eflags));

    // Setup registers for the `iret` inside of `_first_yield`
    m_registers.eflags = eflags;
    m_registers.cs = selector;
    m_registers.eip = reinterpret_cast<u32>(m_entry);

    m_registers.cr3 = this->page_directory()->cr3();
    m_registers.ebp = m_user_stack.base();

    m_registers.ds = segment;
    m_registers.es = segment;
    m_registers.fs = segment;
    m_registers.gs = segment;

    // If we are returning to a different privilege level, we need to push `ss` and `esp` onto the stack for the iret
    if (!this->is_kernel()) {
        m_kernel_stack.push(0x23);
        m_kernel_stack.push(m_user_stack.value());
    }

    // Push the registers onto the stack
    m_kernel_stack.push(m_registers.eflags);
    m_kernel_stack.push(m_registers.cs);
    m_kernel_stack.push(m_registers.eip);

    m_kernel_stack.push(m_registers.eax);
    m_kernel_stack.push(m_registers.ecx);
    m_kernel_stack.push(m_registers.edx);
    m_kernel_stack.push(m_registers.ebx);
    m_kernel_stack.push(0); // `popad` increments ESP by 4 here so we push a dummy value
    m_kernel_stack.push(m_registers.ebp);
    m_kernel_stack.push(m_registers.esi);
    m_kernel_stack.push(m_registers.edi);

    m_kernel_stack.push(m_registers.ds);
    m_kernel_stack.push(m_registers.es);
    m_kernel_stack.push(m_registers.fs);
    m_kernel_stack.push(m_registers.gs);

    m_kernel_stack.push(reinterpret_cast<u32>(_first_yield));
    m_kernel_stack.push(m_registers.ebx); 
    m_kernel_stack.push(m_registers.esi);
    m_kernel_stack.push(m_registers.edi);
    m_kernel_stack.push(0); // ebp

    m_registers.esp0 = m_kernel_stack.value();
    m_registers.esp = m_user_stack.base();
}

void Thread::exit(void* value) {
    m_exit_value = value;
    m_state = Dead;

    m_process->notify_exit(this);
    if (Scheduler::next_thread() == this) {
        Scheduler::set_next_thread(next);
    }

    if (prev && prev->next == this) {
        prev->next = next;
    } if (next) {
        next->prev = prev;
    }

    Scheduler::yield();
}

bool Thread::should_unblock() const {
    return m_blocker && m_blocker->should_unblock();
}

void Thread::block(Blocker* blocker) {
    m_blocker = blocker;
    m_state = Blocked;

    Scheduler::yield();
}

void Thread::unblock() {
    m_blocker = nullptr;
    m_state = Running;

    Scheduler::queue(this);
}

void Thread::enqueue(Thread* thread) {
    if (thread == this) {
        return;
    }

    if (next) {
        next->enqueue(thread);
    } else {
        thread->prev = this;
        next = thread;
    }
}

Thread* Thread::take_next() {
    Thread* next_thread = next;
    next = nullptr;

    return next_thread;
}

}