#include <kernel/cpu/idt.h>
#include <kernel/serial.h>
#include <kernel/memory/mm.h>
#include <kernel/panic.h>

#include <std/cstring.h>

namespace kernel::cpu {

static IDTEntry s_idt_entries[256];

extern "C" void* _isr_stub_table[];
extern "C" void _default_interrupt_handler();

extern "C" void _interrupt_exception_handler(cpu::Registers* regs) {
    switch (regs->intno) {
        case 14:
            memory::MemoryManager::page_fault_handler(regs);
            break;
        case 13:
            serial::printf("\n\nGeneral protection fault at EIP=%#x\n", regs->eip);
            serial::printf("  Error code: %#x\n\n", regs->errno);

            kernel::panic("General protection fault", false);
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
    std::memset(s_idt_entries, 0, sizeof(s_idt_entries));

    IDTPointer idt = {
        .limit = sizeof(s_idt_entries) - 1,
        .base = reinterpret_cast<u32>(&s_idt_entries)
    };

    for (u16 i = 0; i < 32; i++) {
        set_idt_entry(i, reinterpret_cast<u32>(_isr_stub_table[i]), INTERRUPT_GATE);
    }

    asm volatile("lidt %0" : : "m"(idt));
}

}