#include <kernel/arch/processor.h>

#include <kernel/arch/x86_64/gdt.h>
#include <kernel/arch/x86_64/idt.h>

#include <kernel/process/scheduler.h>
#include <kernel/process/process.h>

namespace kernel {

extern "C" void _syscall_handler(arch::Registers* regs) {
    auto* process = Process::current();
    regs->rax = process->handle_syscall(regs);
}
    
extern "C" void _syscall_interrupt_handler();

void Processor::init() {
    auto& processor = instance();
    processor.preinit();
    
    arch::init_gdt();
    arch::init_idt();

    if (processor.has_feature(arch::CPUFeatures::SSE)) {
        arch::write_cr0((arch::read_cr0() & 0xfffffffb) | 2); // Clear CR0.EM and set CR0.MP
        arch::write_cr4(arch::read_cr4() | 0x600);            // Set CR4.OSFXSR and CR4.OSXMMEXCPT
    }

    // Enable and setup the `syscall` instruction
    wmsr(arch::MSR_EFER, rmsr(arch::MSR_EFER) | 1);
    wmsr(arch::MSR_STAR, (0x28ul << 32) | (0x33ul << 48));
    wmsr(arch::MSR_LSTAR, reinterpret_cast<u64>(&_syscall_interrupt_handler));

    // TODO: Setup swapgs
    wmsr(arch::MSR_GS_BASE, reinterpret_cast<u64>(&processor));
}

}