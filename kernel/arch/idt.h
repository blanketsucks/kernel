#pragma once

#include <kernel/common.h>

namespace kernel::arch {

struct IDTDescriptor {
    u16 limit;
    u64 base;
} PACKED;

struct IDTEntry {
    u16 base_low;        // offset bits 0..15
    u16 selector;        // a code segment selector in GDT or LDT
    u8  ist;             // bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
    u8  flags;           // gate type, dpl, and p fields
    u16 base_middle;     // offset bits 16..31
    u32 base_high;       // offset bits 32..63
    u32 zero;
} PACKED;

void set_idt_entry(u16 index, u64 base, u8 flags);
void init_idt();
    
}