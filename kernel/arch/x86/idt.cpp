#include <kernel/arch/x86/idt.h>
#include <kernel/arch/x86/registers.h>
#include <kernel/arch/interrupts.h>
#include <kernel/memory/manager.h>
#include <kernel/serial.h>
#include <kernel/panic.h>

#include <std/cstring.h>

namespace kernel::arch {

void set_interrupt_handler(u32 interrupt, uintptr_t handler, u8 flags) {
    arch::set_idt_entry(interrupt, handler, flags);
}

static IDTEntry s_idt_entries[256];
static IDTPointer s_idt;

extern "C" void* _isr_stub_table[];
extern "C" void _default_interrupt_handler();

extern "C" void _interrupt_exception_handler(arch::InterruptRegisters* regs) {
    switch (regs->intno) {
        case arch::Interrupt::PageFault:
            memory::MemoryManager::page_fault_handler(regs);
            break;
        case arch::Interrupt::GeneralProtectionFault:
            serial::printf("General protection fault at EIP=%#x\n", regs->eip);
            serial::printf("  Error code: %#x\n\n", regs->errno);

            // kernel::panic("General protection fault", false);
            while (1);
        default:
            serial::printf("Unhandled exception: %u\n", regs->intno);
    }
}

void set_idt_entry(u16 index, u32 base, u8 flags) {
    IDTEntry& entry = s_idt_entries[index];

    entry.base_low = base & 0xFFFF;
    entry.base_high = (base >> 16) & 0xFFFF;

    entry.selector = 0x08;
    entry.flags = flags;
    entry.zero = 0;
}

void init_idt() {
    s_idt.base = reinterpret_cast<u32>(&s_idt_entries);
    s_idt.limit = sizeof(s_idt_entries) - 1;

    memset(s_idt_entries, 0, sizeof(s_idt_entries));
    for (u16 i = 0; i < 32; i++) {
        set_idt_entry(i, reinterpret_cast<u32>(_isr_stub_table[i]), arch::INTERRUPT_GATE);
    }

    asm volatile("lidt %0" :: "m"(s_idt));
}

}