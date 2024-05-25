#include "kernel/arch/x86/page_directory.h"
#include <kernel/process/threads.h>
#include <kernel/process/scheduler.h>
#include <kernel/process/process.h>
#include <kernel/process/blocker.h>

namespace kernel {

extern "C" void _first_yield();

Thread* Thread::create(u32 id, String name, Process* process, Entry entry, ProcessArguments& arguments) {
    return new Thread(move(name), process, id, entry, arguments);
}

Thread* Thread::create(String name, Process* process, Entry entry, ProcessArguments& arguments) {
    return new Thread(move(name), process, generate_id(), entry, arguments);
}

Thread::Thread(
    String name, Process* process, u32 id, Entry entry, ProcessArguments& arguments
) : m_id(id), m_state(Running), m_entry(entry), m_name(move(name)), m_process(process), m_arguments(arguments) {
    this->create_stack();
}

bool Thread::is_kernel() const {
    return m_process->is_kernel();
}

arch::PageDirectory* Thread::page_directory() const {
    return m_process->page_directory();
}

void Thread::create_stack() {
    void* kernel_stack = MM->allocate_kernel_region(KERNEL_STACK_SIZE);
    m_kernel_stack = Stack(kernel_stack, KERNEL_STACK_SIZE);

    u8 cs = 0x08; // Data segment selector
    u8 ss = 0x10;  // Code segment selector

    if (!this->is_kernel()) {
        void* user_stack = m_process->allocate(USER_STACK_SIZE, PageFlags::Write);
        m_user_stack = Stack(user_stack, USER_STACK_SIZE);

        this->setup_thread_arguments();

        cs = 0x1B;
        ss = 0x23;
    } else {
        m_user_stack = m_kernel_stack;
    }

    // Only enable interrupts
    u32 eflags = 0x202;

    // Setup registers for the `iret` inside of `_first_yield`
    m_registers.eflags = eflags;
    m_registers.cs = cs;
    m_registers.eip = reinterpret_cast<u32>(m_entry);

    m_registers.cr3 = this->page_directory()->cr3();
    m_registers.ebp = m_user_stack.value();

    m_registers.ds = ss;
    m_registers.es = ss;
    m_registers.fs = ss;
    m_registers.gs = ss;

    // If we are returning to a different privilege level, we need to push `ss` and `esp` onto the stack for `iret`
    if (!this->is_kernel()) {
        m_kernel_stack.push<u32>(ss);
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
    m_kernel_stack.push<u32>(0); // `popad` increments ESP by 4 here so we push a dummy value
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
    m_kernel_stack.push<u32>(0); // ebp

    m_registers.esp0 = m_kernel_stack.value();
    m_registers.esp = m_user_stack.value();
}

void Thread::setup_thread_arguments() {
    page_directory()->switch_to(); // FIXME: Should we do this?

    Vector<VirtualAddress> argv, envp;

    argv.reserve(m_arguments.argv.size());
    envp.reserve(m_arguments.envp.size());

    auto prepare_argument_vector = [this](Vector<String>& src, Vector<VirtualAddress>& dst) {
        for (auto& argument : src) {
            // FIXME: Maybe we should just use the stack for this?
            void* address = m_process->allocate(argument.size() + 1, PageFlags::Write);
            memcpy(address, argument.data(), argument.size());

            reinterpret_cast<char*>(address)[argument.size()] = '\0';
            dst.append(reinterpret_cast<VirtualAddress>(address));
        }
    };

    prepare_argument_vector(m_arguments.argv, argv);
    prepare_argument_vector(m_arguments.envp, envp);

    m_user_stack.push<u32>(0);
    for (size_t i = argv.size(); i > 0; i--) {
        m_user_stack.push<u32>(argv[i - 1]);
    }

    VirtualAddress argv_address = m_user_stack.value();

    m_user_stack.push<u32>(0);
    for (size_t i = envp.size(); i > 0; i--) {
        m_user_stack.push<u32>(envp[i - 1]);
    }

    VirtualAddress envp_address = m_user_stack.value();
    size_t argc = m_arguments.argv.size();

    m_user_stack.push<u32>(envp_address);
    m_user_stack.push<u32>(argv_address);
    m_user_stack.push<u32>(argc);
    m_user_stack.push<u32>(0xDEADCAFE); // _start return address

    arch::PageDirectory::kernel_page_directory()->switch_to();
}

void Thread::exit(void* value) {
    m_exit_value = value;
    m_process->notify_exit(this);

    this->kill();
    Scheduler::yield();
}

void Thread::kill() {
    m_state = Dead;
    if (Scheduler::next_thread() == this) {
        Scheduler::set_next_thread(next);
    }

    if (prev && prev->next == this) {
        prev->next = next;
    } if (next) {
        next->prev = prev;
    }
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