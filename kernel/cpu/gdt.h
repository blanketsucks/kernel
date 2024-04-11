#pragma once

#include <kernel/common.h>

namespace kernel::cpu {

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

        u8 value;
    } flags;

    u8 base_high;
} PACKED;

struct GDTPointer {
    u16 limit;
    u32 base;
} PACKED;

struct TSS {
    u32 prev_tss; // The previous TSS - with hardware task switching these form a kind of backward linked list.
	u32 esp0;     // The stack pointer to load when changing to kernel mode.
	u32 ss0;      // The stack segment to load when changing to kernel mode.

	// Everything below here is unused.
	u32 esp1; // esp and ss 1 and 2 would be used when switching to rings 1 or 2.
	u32 ss1;
	u32 esp2;
	u32 ss2;
	u32 cr3;
	u32 eip;
	u32 eflags;
	u32 eax;
	u32 ecx;
	u32 edx;
	u32 ebx;
	u32 esp;
	u32 ebp;
	u32 esi;
	u32 edi;
	u32 es;
	u32 cs;
	u32 ss;
	u32 ds;
	u32 fs;
	u32 gs;
	u32 ldt;
	u16 trap;
	u16 iomap_base;
} PACKED;

void init_gdt();
void set_gdt_entry(
    u32 index, u32 base, u32 limit, 
    bool rw, bool executable, u8 privilege,
    u8 type = 1, bool present = true, bool accessed = false
);

void write_tss(u32 index, TSS& tss);

}