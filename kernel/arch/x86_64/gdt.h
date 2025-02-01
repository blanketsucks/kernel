#pragma once

#include <kernel/common.h>

namespace kernel::arch {

struct GDTDescriptor {
    u16 limit;
    u64 base;
} PACKED;

struct GDTEntry {
    u16 limit_low;
    u16 base_low;
    u8 base_middle;

    union {
        struct {
            u8 accessed : 1;
            u8 readable : 1;
            u8 direction : 1;
            u8 executable : 1;
            u8 type : 1;
            u8 privilege : 2;
            u8 present : 1;
        } PACKED;

        u8 value;
    } access;

    union {
        struct {
            u8 limit_high : 4;
            u8 zero : 1;
            u8 long_mode : 1;
            u8 size : 1;
            u8 granularity : 1;
        } PACKED;

        u8 value : 4;
    } flags;

    u8 base_high;
} PACKED;

enum GDTEntrySize {
    GDT_ENTRY_SIZE_16 = 0,
    GDT_ENTRY_SIZE_32 = 1,
    GDT_ENTRY_SIZE_64 = 2
};

void init_gdt();
void set_gdt_entry(
    u32 index, u32 limit, bool is_data_segment, GDTEntrySize size,
    bool rw, bool executable, u8 privilege,
    u8 type = 1, bool present = true, bool accessed = false
);

}