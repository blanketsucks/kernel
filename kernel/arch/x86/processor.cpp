#include <kernel/arch/processor.h>
#include <kernel/process/scheduler.h>
#include <kernel/process/process.h>

#include <kernel/arch/x86/idt.h>
#include <kernel/arch/x86/gdt.h>
#include <kernel/arch/x86/registers.h>

namespace kernel {

extern "C" void _syscall_interrupt_handler(arch::InterruptRegisters*);

extern "C" void _syscall_handler(arch::Registers* regs) {
    auto* process = Scheduler::current_process();
    regs->eax = process->handle_syscall(regs);
}

void Processor::init() {
    auto& processor = instance();
    processor.m_features = arch::cpu_features();

    // TODO: Enable features like SSE and more

    arch::init_gdt();
    arch::init_idt();

    arch::set_idt_entry(0x80, reinterpret_cast<uintptr_t>(_syscall_interrupt_handler), 0xEF);
}

}