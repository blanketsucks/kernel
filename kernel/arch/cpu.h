#pragma once

#include <kernel/common.h>

namespace kernel::arch {

enum class CPUFeatures : u64 {
    SSE3         = 1ull << 0,
    PCLMULQDQ    = 1ull << 1,
    DTES64       = 1ull << 2,
    MONITOR      = 1ull << 3,
    DS_CPL       = 1ull << 4,
    VMX          = 1ull << 5,
    SMX          = 1ull << 6,
    EIST         = 1ull << 7,
    TM2          = 1ull << 8,
    SSSE3        = 1ull << 9,
    CNXT_ID      = 1ull << 10,
    SDBG         = 1ull << 11,
    FMA          = 1ull << 12,
    CMPXCHG16B   = 1ull << 13,
    XTPR         = 1ull << 14,
    PDCM         = 1ull << 15,
    PCID         = 1ull << 17,
    DCA          = 1ull << 18,
    SSE4_1       = 1ull << 19,
    SSE4_2       = 1ull << 20,
    X2APIC       = 1ull << 21,
    MOVBE        = 1ull << 22,
    POPCNT       = 1ull << 23,
    TSC_DEADLINE = 1ull << 24,
    AES          = 1ull << 25,
    XSAVE        = 1ull << 26,
    OSXSAVE      = 1ull << 27,
    AVX          = 1ull << 28,
    F16C         = 1ull << 29,
    RDRAND       = 1ull << 30,

    FPU          = 1ull << 32,
    VME          = 1ull << 33,
    DE           = 1ull << 34,
    PSE          = 1ull << 35,
    TSC          = 1ull << 36,
    MSR          = 1ull << 37,
    PAE          = 1ull << 38,
    MCE          = 1ull << 39,
    CX8          = 1ull << 40,
    APIC         = 1ull << 41,
    SEP          = 1ull << 43,
    MTRR         = 1ull << 44,
    PGE          = 1ull << 45,
    MCA          = 1ull << 46,
    CMOV         = 1ull << 47,
    PSE_36       = 1ull << 48,
    PSN          = 1ull << 49,
    CLFSH        = 1ull << 50,
    DS           = 1ull << 52,
    ACPI         = 1ull << 53,
    MMX          = 1ull << 54,
    FXSR         = 1ull << 55,
    SSE          = 1ull << 56,
    SSE2         = 1ull << 57,
    SS           = 1ull << 58,
    HTT          = 1ull << 59,
    TM           = 1ull << 60,
    PBE          = 1ull << 62,
};

MAKE_ENUM_BITWISE_OPS(CPUFeatures);

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

FlatPtr cpu_flags();

CPUFeatures cpu_features();

}