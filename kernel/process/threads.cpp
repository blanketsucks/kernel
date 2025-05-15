#include <kernel/process/threads.h>
#include <kernel/process/scheduler.h>
#include <kernel/process/process.h>
#include <kernel/process/blocker.h>
#include <kernel/time/manager.h>
#include <std/format.h>

namespace kernel {

Thread* Thread::create(u32 id, String name, Process* process, Entry entry, void* entry_data, ProcessArguments& arguments) {
    return new Thread(move(name), process, id, entry, entry_data, arguments);
}

Thread* Thread::create(String name, Process* process, Entry entry, void* entry_data, ProcessArguments& arguments) {
    return new Thread(move(name), process, generate_id(), entry, entry_data, arguments);
}

Thread* Thread::current() {
    return Scheduler::current_thread();
}

Thread::Thread(
    String name, Process* process, pid_t id, Entry entry, void* entry_data, ProcessArguments& arguments
) : m_id(id), m_state(Running), m_entry(entry), m_entry_data(entry_data), m_name(move(name)), m_process(process), m_arguments(arguments) {
    this->create_stack();
}

Thread::Thread(
    Process* process, arch::Registers& registers
) : m_id(process->id()), m_state(Running), m_name("main"), m_process(process), m_arguments(process->m_arguments) {
    void* kernel_stack = MM->allocate_kernel_region(KERNEL_STACK_SIZE);
    m_kernel_stack = Stack(kernel_stack, KERNEL_STACK_SIZE);

    memcpy(&m_registers, &registers, sizeof(arch::Registers));
    m_registers.cr3 = page_directory()->cr3();

    m_registers.set_syscall_return(0);
    this->set_initial_stack_state(m_registers.sp(), m_registers);
}

pid_t Thread::pid() const {
    return m_process->id();
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

    bool is_kernel_process = this->is_kernel();
    if (!is_kernel_process) {
        void* user_stack = m_process->allocate(USER_STACK_SIZE, PageFlags::Write);
        m_user_stack = Stack(user_stack, USER_STACK_SIZE);

        this->setup_thread_arguments();

        m_registers.cs = arch::USER_CODE_SELECTOR | 3;
        m_registers.ss = arch::USER_DATA_SELECTOR | 3;
    } else {
        m_user_stack = m_kernel_stack;

        m_registers.cs = arch::KERNEL_CODE_SELECTOR;
        m_registers.ss = arch::KERNEL_DATA_SELECTOR;

        m_registers.set_entry_data(reinterpret_cast<FlatPtr>(m_entry_data));
    }

    FlatPtr kernel_stack_top = m_kernel_stack.top();
    FlatPtr user_stack_top = m_user_stack.top();

    // Only enable interrupts
    FlatPtr flags = 0x202;
    m_registers.set_flags(flags);

    m_registers.cr3 = page_directory()->cr3();
    if (is_kernel_process) {
        m_registers.set_bp(kernel_stack_top);
    } else {
        m_registers.set_bp(user_stack_top);
    }

    m_registers.set_ip(reinterpret_cast<FlatPtr>(m_entry));

    this->set_initial_stack_state(m_user_stack.value(), m_registers);
}

void Thread::set_initial_stack_state(FlatPtr sp, arch::ThreadRegisters& registers) {
    m_registers.set_user_sp(sp);
    registers.set_initial_stack_state(this);
}

void Thread::setup_thread_arguments() {
    page_directory()->switch_to(); // FIXME: Should we do this?

    Vector<VirtualAddress> argv, envp;

    argv.reserve(m_arguments.argv.size());
    envp.reserve(m_arguments.envp.size());

    auto prepare_argument_vector = [this](Vector<String>& src, Vector<VirtualAddress>& dst) {
        for (auto& argument : src) {
            if (argument.empty()) {
                dst.append(0);
                continue;
            }

            // FIXME: Maybe we should just use the stack for this?
            void* address = m_process->allocate(argument.size() + 1, PageFlags::Write);
            memcpy(address, argument.data(), argument.size());

            reinterpret_cast<char*>(address)[argument.size()] = '\0';
            dst.append(reinterpret_cast<VirtualAddress>(address));
        }

        dst.append(0); // Null-terminate the argument vector (argv/envp)
    };

    prepare_argument_vector(m_arguments.argv, argv);
    prepare_argument_vector(m_arguments.envp, envp);

    for (size_t i = argv.size(); i > 0; i--) {
        m_user_stack.push<FlatPtr>(argv[i - 1]);
    }

    VirtualAddress argv_address = m_user_stack.value();
    for (size_t i = envp.size(); i > 0; i--) {
        m_user_stack.push<FlatPtr>(envp[i - 1]);
    }

    VirtualAddress envp_address = m_user_stack.value();
    size_t argc = m_arguments.argv.size();

#ifdef __x86_64__
    m_registers.rdi = argc;
    m_registers.rsi = argv_address;
    m_registers.rdx = envp_address;
#else
    m_user_stack.push<FlatPtr>(envp_address);
    m_user_stack.push<FlatPtr>(argv_address);
    m_user_stack.push<FlatPtr>(argc);
#endif

    m_user_stack.push<FlatPtr>(0xdeadbeef); // _start return address

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
    if (m_state != Blocked) {
        return false;
    }

    return m_blocker && m_blocker->should_unblock();
}

void Thread::block(Blocker* blocker) {
    ScopedSchedulerLock lock;

    m_blocker = blocker;
    m_state = Blocked;

    Scheduler::yield();
}

void Thread::unblock() {
    m_blocker = nullptr;
    m_state = Running;
    
    Scheduler::queue(this);
}   

void Thread::sleep(clockid_t clock_id, const Duration& duration) {
    auto* blocker = new SleepBlocker(duration, clock_id);
    this->block(blocker);
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