#include <kernel/arch/processor.h>

#include <kernel/arch/x86_64/msr.h>
#include <kernel/arch/x86_64/gdt.h>
#include <kernel/arch/x86_64/idt.h>

#include <kernel/process/scheduler.h>
#include <kernel/process/process.h>

namespace kernel {

extern "C" void _syscall_handler(arch::Registers* regs) {
    auto* process = Scheduler::current_process();
    regs->rax = process->handle_syscall(regs);
}
    
extern "C" void _syscall_interrupt_handler();

void Processor::init() {
    auto& processor = instance();
    processor.m_features = arch::cpu_features();
    
    arch::init_gdt();
    arch::init_idt();

    ASSERT(processor.has_feature(arch::CPUFeatures::SEP), "`syscall` instruction is not supported");
    if (processor.has_feature(arch::CPUFeatures::SSE)) {
        asm volatile(
            "mov %cr0, %rax\n"
            "and $0xFFFB, %ax\n"
            "or $2, %ax\n"
            "mov %rax, %cr0\n"

            "mov %cr4, %rax\n"
            "or $0x600, %ax\n"
            "mov %rax, %cr4"
        );
    }

    // Enable and setup the `syscall` instruction
    wmsr(arch::MSR_EFER, rmsr(arch::MSR_EFER) | 1);
    wmsr(arch::MSR_STAR, (0x28ul << 32) | (0x33ul << 48));
    wmsr(arch::MSR_LSTAR, reinterpret_cast<u64>(&_syscall_interrupt_handler));

    // TODO: Setup swapgs
    wmsr(arch::MSR_GS_BASE, reinterpret_cast<u64>(&processor));
}

}