#pragma once

#include <kernel/common.h>

#define ENUMERATE_CPU_FEATURES(Op)              \
    Op(SSE3, "sse3", (1ull << 0))              \
    Op(PCLMULQDQ, "pclmulqdq", (1ull << 1))    \
    Op(DTES64, "dtes64", (1ull << 2))          \
    Op(MONITOR, "monitor", (1ull << 3))        \
    Op(DS_CPL, "ds_cpl", (1ull << 4))          \
    Op(VMX, "vmx", (1ull << 5))                \
    Op(SMX, "smx", (1ull << 6))                \
    Op(EIST, "eist", (1ull << 7))              \
    Op(TM2, "tm2", (1ull << 8))                \
    Op(SSSE3, "ssse3", (1ull << 9))            \
    Op(CNXT_ID, "cnxt_id", (1ull << 10))       \
    Op(SDBG, "sdbg", (1ull << 11))             \
    Op(FMA, "fma", (1ull << 12))               \
    Op(CMPXCHG16B, "cmpxchg16b", (1ull << 13)) \
    Op(XTPR, "xtpr", (1ull << 14))             \
    Op(PDCM, "pdcm", (1ull << 15))             \
    Op(PCID, "pcid", (1ull << 17))             \
    Op(DCA, "dca", (1ull << 18))               \
    Op(SSE4_1, "sse4.1", (1ull << 19))         \
    Op(SSE4_2, "sse4.2", (1ull << 20))         \
    Op(X2APIC, "x2apic", (1ull << 21))         \
    Op(MOVBE, "movbe", (1ull << 22))           \
    Op(POPCNT, "popcnt", (1ull << 23))         \
    Op(TSC_DEADLINE, "tsc_deadline", (1ull << 24)) \
    Op(AES, "aes", (1ull << 25))               \
    Op(XSAVE, "xsave", (1ull << 26))           \
    Op(OSXSAVE, "osxsave", (1ull << 27))       \
    Op(AVX, "avx", (1ull << 28))               \
    Op(F16C, "f16c", (1ull << 29))             \
    Op(RDRAND, "rdrand", (1ull << 30))         \
    Op(FPU, "fpu", (1ull << 32))               \
    Op(VME, "vme", (1ull << 33))               \
    Op(DE, "de", (1ull << 34))                 \
    Op(PSE, "pse", (1ull << 35))               \
    Op(TSC, "tsc", (1ull << 36))               \
    Op(MSR, "msr", (1ull << 37))               \
    Op(PAE, "pae", (1ull << 38))               \
    Op(MCE, "mce", (1ull << 39))               \
    Op(CX8, "cx8", (1ull << 40))               \
    Op(APIC, "apic", (1ull << 41))             \
    Op(SEP, "sep", (1ull << 43))               \
    Op(MTRR, "mtrr", (1ull << 44))             \
    Op(PGE, "pge", (1ull << 45))               \
    Op(MCA, "mca", (1ull << 46))               \
    Op(CMOV, "cmov", (1ull << 47))             \
    Op(PSE_36, "pse_36", (1ull << 48))         \
    Op(PSN, "psn", (1ull << 49))               \
    Op(CLFSH, "clfsh", (1ull << 50))           \
    Op(DS, "ds", (1ull << 52))                 \
    Op(ACPI, "acpi", (1ull << 53))             \
    Op(MMX, "mmx", (1ull << 54))               \
    Op(FXSR, "fxsr", (1ull << 55))             \
    Op(SSE, "sse", (1ull << 56))               \
    Op(SSE2, "sse2", (1ull << 57))             \
    Op(SS, "ss", (1ull << 58))                 \
    Op(HTT, "htt", (1ull << 59))               \
    Op(TM, "tm", (1ull << 60))                 \
    Op(PBE, "pbe", (1ull << 62))


namespace kernel::arch {

enum class CPUFeatures : u64 {
#define Op(name, _, value) name = value,
    ENUMERATE_CPU_FEATURES(Op)
#undef Op
};

MAKE_ENUM_BITWISE_OPS(CPUFeatures);

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

void cpuid(u32 leaf, u32& eax, u32& ebx, u32& ecx, u32& edx);

void wmsr(u32 msr, u64 value);
u64 rmsr(u32 msr);

FlatPtr cpu_flags();

CPUFeatures cpu_features();

FlatPtr read_cr0();
FlatPtr read_cr2();
FlatPtr read_cr3();
FlatPtr read_cr4();

void write_cr0(FlatPtr value);
void write_cr2(FlatPtr value);
void write_cr3(FlatPtr value);
void write_cr4(FlatPtr value);

}