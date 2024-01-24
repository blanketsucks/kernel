#pragma once

#include <kernel/common.h>

namespace kernel::cpu {

struct IDTEntry {
    u16 base_low;
    u16 selector;
    u8 zero;
    u8 flags;
    u16 base_high;
} PACKED;

struct IDTPointer {
    u16 limit;
    u32 base;
} PACKED;

static_assert(sizeof(IDTPointer) == 6, "IDTPointer must be 6 bytes long");

struct InterruptFrame {
    u32 eip;
    u32 cs;
    u32 flags;
    u32 sp;
    u32 ss;
} PACKED;

void init_idt();
void set_idt_entry(u8 index, u32 base, u8 flags);

}