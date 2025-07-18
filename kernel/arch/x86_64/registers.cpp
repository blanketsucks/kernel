#include <kernel/arch/x86_64/registers.h>
#include <kernel/process/stack.h>
#include <kernel/process/threads.h>

#include <std/format.h>

namespace kernel::arch {

extern "C" void _thread_first_enter();

extern "C" void _thread_context_init(Thread*) {}

void ThreadRegisters::set_initial_stack_state(Thread* thread) {
    auto& stack = thread->kernel_stack();

    stack.push(ss);
    stack.push(rsp);
    stack.push(rflags);
    stack.push(cs);
    stack.push(rip);

    stack.push(rax);
    stack.push(rbx);
    stack.push(rcx);
    stack.push(rdx);
    stack.push<FlatPtr>(0);
    stack.push(rbp);
    stack.push(rdi);
    stack.push(rsi);
    stack.push(r8);
    stack.push(r9);
    stack.push(r10);
    stack.push(r11);
    stack.push(r12);
    stack.push(r13);
    stack.push(r14);
    stack.push(r15);

    // Argument for _thread_context_init
    stack.push(reinterpret_cast<FlatPtr>(thread));
    stack.push(reinterpret_cast<FlatPtr>(&_thread_first_enter));
    
    stack.push<FlatPtr>(0x2); // Start the thread with interrupts disabled
    stack.push(r15);
    stack.push(r14);
    stack.push(r13);
    stack.push(r12);
    stack.push<FlatPtr>(0);
    stack.push(rbx);

    rsp = stack.value();
    rsp0 = stack.top();
}

}