#include <kernel/cpu/idt.h>
#include <kernel/vga.h>

namespace kernel::cpu {

static IDTEntry idt[256];

INTERRUPT void default_int_handler(InterruptFrame* frame) {
    (void)frame;
}

INTERRUPT void default_int_hander_with_code(InterruptFrame* frame, u32 error_code) {
    (void)frame;
    (void)error_code;

    while (true) {}
}

void set_idt_entry(u8 index, u32 base, u8 flags) {
    IDTEntry& entry = idt[index];

    entry.base_low = base & 0xFFFF;
    entry.base_high = (base >> 16) & 0xFFFF;

    entry.selector = 0x08;
    entry.flags = flags;
    entry.zero = 0;
}

void init_idt() {
    IDTPointer ip {
        .limit = sizeof(idt) - 1,
        .base = reinterpret_cast<u32>(&idt)
    };

    for (u32 i = 0; i < 32; i++) {
        if (i == 8 || i == 10 || i == 11 || i == 12 || i == 13 || i == 14 || i == 17 || i == 21) {
            set_idt_entry(i, reinterpret_cast<u32>(default_int_hander_with_code), 0x8F);
            continue;
        }

        set_idt_entry(i, reinterpret_cast<u32>(default_int_handler), 0x8F);
    }

    for (u32 i = 32; i < 256; i++) {
        set_idt_entry(i, reinterpret_cast<u32>(default_int_handler), 0x8E);
    }

    asm volatile("lidt %0" : : "memory"(ip));
}

}