#pragma once

#include <kernel/common.h>

namespace kernel::arch {

struct GPRegisters {
    u64 r15, r14, r13, r12, r11, r10, r9, r8;
    u64 rsi, rdi, rbp, rdx, rcx, rbx, rax;
} PACKED;

struct InterruptRegisters {
    u64 r15, r14, r13, r12, r11, r10, r9, r8;
    u64 rsi, rdi, rbp, rdx, rcx, rbx, rax;
    u64 intno, errno;
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