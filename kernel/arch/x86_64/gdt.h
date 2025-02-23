#pragma once

#include <kernel/common.h>

namespace kernel::arch {

class TSS;

struct GDTDescriptor {
    u16 limit;
    u64 base;
} PACKED;

union GDTEntry {
    struct {
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
                u8 : 1;
                u8 long_mode : 1;
                u8 size : 1;
                u8 granularity : 1;
            } PACKED;

            u8 value;
        } flags;

        u8 base_high;
    };

    struct {
        u32 low;
        u32 high;
    };

    void set_base(u64 base) {
        base_low = base & 0xFFFF;
        base_middle = (base >> 16) & 0xFF;
        base_high = (base >> 24) & 0xFF;
    }

    void set_limit(u32 limit) {
        limit_low = limit & 0xFFFF;
        flags.limit_high = (limit >> 16) & 0x0F;
    }
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

void write_tss(u16 index, TSS&);

}