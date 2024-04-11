#pragma once

#include <kernel/common.h>

namespace kernel::cpu {

constexpr u8 TRAP_GATE = 0x8F;
constexpr u8 INTERRUPT_GATE = 0x8E;

struct IDTEntry {
    u16 base_low;
    u16 selector;
    u8 zero;

    union {
        struct {
            u8 gate : 4;
            u8 privilege : 2;
            u8 present : 1;
        };

        u8 flags;
    };

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

struct Registers {
    u32 gs, fs, es, ds;
    u32 edi, esi, ebp, esp0, ebx, edx, ecx, eax;
    u32 intno, errno;
    u32 eip, cs, eflags, esp, ss;
} PACKED;

using InterruptHandler = void(*)(InterruptFrame*);

void init_idt();
void set_idt_entry(u16 index, u32 base, u8 flags);

}