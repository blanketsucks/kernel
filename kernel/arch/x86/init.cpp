#include <kernel/arch/arch.h>
#include <kernel/arch/x86/registers.h>
#include <kernel/arch/x86/idt.h>
#include <kernel/arch/x86/gdt.h>

namespace kernel::arch {

extern "C" void _syscall_interrupt_handler(arch::InterruptRegisters*);
extern "C" void _syscall_handler(arch::Registers* regs) {}

void init() {
    arch::init_gdt();
    arch::init_idt();

    arch::set_idt_entry(0x80, reinterpret_cast<uintptr_t>(_syscall_interrupt_handler), 0xEF);
}

}