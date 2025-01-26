#pragma once

#include <kernel/common.h>

namespace kernel::arch {

struct InterruptRegisters {
    u32 gs, fs, es, ds;
    u32 edi, esi, ebp, esp0, ebx, edx, ecx, eax;
    u32 intno, errno;
    u32 eip, cs, eflags, esp, ss;
} PACKED;

struct InterruptFrame {
    u32 eip, cs, eflags, esp, ss;
} PACKED;

struct Registers {
    u32 gs, fs, es, ds;
    u32 edi, esi, ebp, esp0, ebx, edx, ecx, eax;
    u32 eip, cs, eflags, esp, ss;
} PACKED;

void dump_registers(const Registers&);
void dump_registers(const InterruptRegisters&);

}