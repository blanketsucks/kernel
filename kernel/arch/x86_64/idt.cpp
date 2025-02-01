#include <kernel/arch/x86_64/idt.h>
#include <kernel/arch/x86_64/registers.h>
#include <kernel/arch/interrupts.h>
#include <kernel/memory/manager.h>

#include <std/cstring.h>
#include <std/format.h>

namespace kernel::arch {

void set_interrupt_handler(u32 interrupt, uintptr_t handler, u8 flags) {
    set_idt_entry(interrupt, handler, flags);
}

static IDTEntry s_idt_entries[256];

extern "C" void* _isr_stub_table[];
extern "C" void _default_interrupt_handler();

extern "C" void _interrupt_exception_handler(InterruptRegisters* regs) {
    switch (regs->intno) {
        case Interrupt::PageFault:
            memory::MemoryManager::page_fault_handler(regs);
            break;
        default:
            dbgln("Unhandled exception: {} (RIP={:#x})", regs->intno, regs->rip);
            dbgln("  Error code: {:#x}", regs->errno);
            dbgln("  Halting...");

            while (1);
    }
}

void set_idt_entry(u16 index, u64 base, u8 flags) {
    IDTEntry& entry = s_idt_entries[index];
    memset(&entry, 0, sizeof(IDTEntry));

    entry.base_low = base & 0xFFFF;
    entry.base_middle = (base >> 16) & 0xFFFF;
    entry.base_high = (base >> 32) & 0xFFFFFFFF;

    entry.selector = 0x28;
    entry.flags = flags;
}

void init_idt() {
    IDTDescriptor idtr;

    idtr.limit = sizeof(s_idt_entries) - 1;
    idtr.base = reinterpret_cast<u64>(&s_idt_entries);

    for (size_t i = 0; i < 32; i++) {
        set_idt_entry(i, reinterpret_cast<u64>(_isr_stub_table[i]), arch::INTERRUPT_GATE);
    }

    asm volatile("lidt %0" :: "m"(idtr));
}

}