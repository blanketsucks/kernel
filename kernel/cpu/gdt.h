#pragma once

#include <kernel/common.h>

namespace kernel::cpu {

struct GDTEntry {
    u16 limit_low;
    u16 base_low;
    u8 base_middle;
    u8 access;
    u8 granularity;
    u8 base_high;
} PACKED;

struct PACKED GDTPointer {
    u16 limit;
    u32 base;
} PACKED;

void init_gdt();
void set_gdt_entry(u32 index, u32 base, u32 limit, u8 access, u8 granularity);

}