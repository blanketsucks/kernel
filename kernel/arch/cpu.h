#pragma once

#include <kernel/common.h>

#define IRQ_VECTOR_BASE   0x20
#define IRQ_HANDLER_COUNT (256 - IRQ_VECTOR_BASE) 

namespace kernel::arch {

struct FPUState {
    u8 state[512];
} ALIGNED(16);

enum {
    MSR_EFER           = 0xC0000080,
    MSR_STAR           = 0xC0000081,
    MSR_LSTAR          = 0xC0000082,
    MSR_CSTAR          = 0xC0000083,
    MSR_SFMASK         = 0xC0000084,
    MSR_FS_BASE        = 0xC0000100,
    MSR_GS_BASE        = 0xC0000101,
    MSR_KERNEL_GS_BASE = 0xC0000102
};

// The bits of rflags and eflags are the same
union Flags {
    struct {
        u8 cf : 1;
        u8 : 1;
        u8 pf : 1;
        u8 : 1;
        u8 af : 1;
        u8 : 1;
        u8 zf : 1;
        u8 sf : 1;
        u8 tf : 1;
        u8 if_ : 1;
        u8 df : 1;
        u8 of : 1;
        u8 iopl : 2;
        u8 nt : 1;
        u8 : 1;
        u8 rf : 1;
        u8 vm : 1;
        u8 ac : 1;
        u8 vif : 1;
        u8 vip : 1;
        u8 id : 1;
    } PACKED;

    FlatPtr value;

    Flags() : value(0) {}
    Flags(FlatPtr value) : value(value) {}
};

void wrmsr(u32 msr, u64 value);
u64  rdmsr(u32 msr);

FlatPtr cpu_flags();

static inline FlatPtr read_gs() {
    return rdmsr(MSR_GS_BASE);
}

static inline void write_gs(FlatPtr value) {
    wrmsr(MSR_GS_BASE, value);
}

static inline FlatPtr read_cr0() {
    FlatPtr cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));

    return cr0;
}

static inline FlatPtr read_cr2() {
    FlatPtr cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));

    return cr2;
}

static inline FlatPtr read_cr3() {
    FlatPtr cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));

    return cr3;
}

static inline FlatPtr read_cr4() {
    FlatPtr cr4;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));

    return cr4;
}

static inline void write_cr0(FlatPtr value) {
    asm volatile("mov %0, %%cr0" :: "r"(value));
}

static inline void write_cr2(FlatPtr value) {
    asm volatile("mov %0, %%cr2" :: "r"(value));
}

static inline void write_cr3(FlatPtr value) {
    asm volatile("mov %0, %%cr3" :: "r"(value));
}

static inline void write_cr4(FlatPtr value) {
    asm volatile("mov %0, %%cr4" :: "r"(value));
}

static inline void invlpg(FlatPtr address) {
    asm volatile("invlpg (%0)" :: "r"(address) : "memory");
}

static inline void fxsave(FPUState& state) {
    asm volatile("fxsave %0" :: "m"(state) : "memory");
}

static inline void fxrstor(FPUState& state) {
    asm volatile("fxrstor %0" :: "m"(state) : "memory");
}

}