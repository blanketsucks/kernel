#pragma once

#include <kernel/common.h>

namespace kernel::arch {

struct InterruptRegisters {
    u32 gs, fs, es, ds;
    u64 rdi, rsi, rbp, rsp0, rbx, rdx, rcx, rax;
    u32 intno, errno;
    u64 rip, cs, eflags, rsp, ss;
} PACKED;

struct InterruptFrame {
    u64 rip, cs, eflags, rsp, ss;
} PACKED;

struct Registers {
    u32 gs, fs, es, ds;
    u64 rdi, rsi, rbp, rsp0, rbx, rdx, rcx, rax;
    u64 rip, cs, eflags, rsp, ss;
} PACKED;

}