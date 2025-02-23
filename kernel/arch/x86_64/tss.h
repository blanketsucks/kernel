#pragma once

#include <kernel/common.h>
#include <kernel/process/threads.h>

namespace kernel::arch {

struct TSS {
    u32 reserved0;

    u32 rsp0l;
    u32 rsp0h;
    u32 rsp1l;
    u32 rsp1h;
    u32 rsp2l;
    u32 rsp2h;

    u64 reserved1;

    u32 ist1l;
    u32 ist1h;
    u32 ist2l;
    u32 ist2h;
    u32 ist3l;
    u32 ist3h;
    u32 ist4l;
    u32 ist4h;
    u32 ist5l;
    u32 ist5h;
    u32 ist6l;
    u32 ist6h;
    u32 ist7l;
    u32 ist7h;

    u64 reserved2;
    u16 reserved3;

    u16 iomap_base;

    void init(Thread* initial_thread) {
        auto& regs = initial_thread->registers();
        iomap_base = sizeof(TSS);

        rsp0l = regs.sp0() & 0xFFFFFFFF;
        rsp0h = regs.sp0() >> 32;
    }

    void set_kernel_stack(FlatPtr stack) {
        rsp0l = stack & 0xFFFFFFFF;
        rsp0h = stack >> 32;
    }
} PACKED;

}