#pragma once

#include <kernel/common.h>

namespace kernel::arch {

struct Registers {
    u32 gs, fs, es, ds;
    u64 rdi, rsi, rbp, rsp0, rbx, rdx, rcx, rax;
    u32 intno, errno;
    u64 rip;
    u32 cs, eflags;
    u64 rsp;
    u32 ss;
} PACKED;

}