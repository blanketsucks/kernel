#include <kernel/arch/processor.h>
#include <kernel/process/process.h>
#include <kernel/common.h>

namespace kernel {

extern "C" void _syscall_handler(arch::Registers* regs) {
    auto* process = Process::current();
    regs->rax = process->handle_syscall(regs);
}

void Processor::syscall_interrupt_handler() {
    asm(
        "mov    %%rsp, %%gs:%c[user_stack_offset]\n"
        "mov    %%gs:%c[kernel_stack_offset], %%rsp\n"
        "pushq  $%c[ds]\n"
        "pushq  %%gs:%c[user_stack_offset]\n"
        "pushq  %%r11\n"
        "pushq  $%c[cs]\n"
        "pushq  %%rcx\n"
        "sti\n"
        "push   %%rax\n"
        "push   %%rbx\n"
        "push   %%rcx\n"
        "push   %%rdx\n"
        "push   %%rsp\n"
        "push   %%rbp\n"
        "push   %%rdi\n"
        "push   %%rsi\n"
        "push   %%r8\n"
        "push   %%r9\n"
        "push   %%r10\n"
        "push   %%r11\n"
        "push   %%r12\n"
        "push   %%r13\n"
        "push   %%r14\n"
        "push   %%r15\n"
        "mov    %%rsp, %%rdi\n"
        "call   _syscall_handler\n"
        "pop    %%r15\n"
        "pop    %%r14\n"
        "pop    %%r13\n"
        "pop    %%r12\n"
        "pop    %%r11\n"
        "pop    %%r10\n"
        "pop    %%r9\n"
        "pop    %%r8\n"
        "pop    %%rsi\n"
        "pop    %%rdi\n"
        "pop    %%rbp\n"
        "add    $8, %%rsp\n"
        "pop    %%rdx\n"
        "pop    %%rcx\n"
        "pop    %%rbx\n"
        "pop    %%rax\n"
        "pop    %%rcx\n"
        "add    $8, %%rsp\n"
        "pop    %%r11\n"
        "cli\n"
        "pop    %%rsp\n"
        "sysretq\n\t"
        ::
        [kernel_stack_offset] "i"(__builtin_offsetof(Processor, m_tss.rsp0l)),
        [user_stack_offset]   "i"(__builtin_offsetof(Processor, m_user_stack)),
        [ds]                  "i"(arch::USER_DATA_SELECTOR | 3),
        [cs]                  "i"(arch::USER_CODE_SELECTOR | 3)
    );
}

}