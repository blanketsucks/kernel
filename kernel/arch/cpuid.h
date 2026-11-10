#pragma once

#include <kernel/common.h>

#include <std/string_view.h>

namespace kernel::arch {

struct CPUID {
    u32 eax;
    u32 ebx;
    u32 ecx;
    u32 edx;
};

enum class CPUIDReg : u8 { eax, ebx, ecx, edx };

struct CPUIDWord {
    u32 leaf;
    CPUIDReg reg;
};

constexpr u32 EXTENDED_LEAF = 0x80000000;

inline constexpr CPUIDWord CPUID_WORDS[] = {
    {0x00000001, CPUIDReg::edx},
    {0x00000001, CPUIDReg::ecx},
    {0x00000007, CPUIDReg::ebx},
    {0x00000007, CPUIDReg::ecx},
    {0x00000007, CPUIDReg::edx},
    {0x80000001, CPUIDReg::edx},
    {0x80000001, CPUIDReg::ecx},
};

inline constexpr size_t WORD_COUNT = sizeof(CPUID_WORDS) / sizeof(CPUIDWord);

consteval u32 feature(u32 word, u32 bit) {
    return word * 32 + bit;
}

enum class Feature : u32 {
    // 01H.EDX
    FPU   = feature(0, 0),
    VME   = feature(0, 1),
    DE    = feature(0, 2),
    PSE   = feature(0, 3),
    TSC   = feature(0, 4),
    MSR   = feature(0, 5),
    PAE   = feature(0, 6),
    MCE   = feature(0, 7),
    CX8   = feature(0, 8),
    APIC  = feature(0, 9),
    SEP   = feature(0, 11),
    MTRR  = feature(0, 12),
    PGE   = feature(0, 13),
    MCA   = feature(0, 14),
    CMOV  = feature(0, 15),
    PAT   = feature(0, 16),
    PSE36 = feature(0, 17),
    PSN   = feature(0, 18),
    CLFSH = feature(0, 19),
    DS    = feature(0, 21),
    ACPI  = feature(0, 22),
    MMX   = feature(0, 23),
    FXSR  = feature(0, 24),
    SSE   = feature(0, 25),
    SSE2  = feature(0, 26),
    SS    = feature(0, 27),
    HTT   = feature(0, 28),
    TM    = feature(0, 29),
    PBE   = feature(0, 31),

    // 01H.ECX
    SSE3         = feature(1, 0),
    PCLMULQDQ    = feature(1, 1),
    DTES64       = feature(1, 2),
    MONITOR      = feature(1, 3),
    DS_CPL       = feature(1, 4),
    VMX          = feature(1, 5),
    SMX          = feature(1, 6),
    EIST         = feature(1, 7),
    TM2          = feature(1, 8),
    SSSE3        = feature(1, 9),
    CNXT_ID      = feature(1, 10),
    SDBG         = feature(1, 11),
    FMA          = feature(1, 12),
    CMPXCHG16B   = feature(1, 13),
    XTPR         = feature(1, 14),
    PDCM         = feature(1, 15),
    PCID         = feature(1, 17),
    DCA          = feature(1, 18),
    SSE4_1       = feature(1, 19),
    SSE4_2       = feature(1, 20),
    X2APIC       = feature(1, 21),
    MOVBE        = feature(1, 22),
    POPCNT       = feature(1, 23),
    TSC_DEADLINE = feature(1, 24),
    AESNI        = feature(1, 25),
    XSAVE        = feature(1, 26),
    OSXSAVE      = feature(1, 27),
    AVX          = feature(1, 28),
    F16C         = feature(1, 29),
    RDRAND       = feature(1, 30),
    HYPERVISOR   = feature(1, 31),

    // 07H.0.EBX
    FSGSBASE             = feature(2, 0),
    TSC_ADJUST           = feature(2, 1),
    SGX                  = feature(2, 2),
    BMI1                 = feature(2, 3),
    HLE                  = feature(2, 4),
    AVX2                 = feature(2, 5),
    FDP_EXCPTN_ONLY      = feature(2, 6),
    SMEP                 = feature(2, 7),
    BMI2                 = feature(2, 8),
    ERMS                 = feature(2, 9),
    INVPCID              = feature(2, 10),
    RTM                  = feature(2, 11),
    RDT_M                = feature(2, 12),
    FPU_CS_DS_DEPRECATED = feature(2, 13),
    MPX                  = feature(2, 14),
    RDT_A                = feature(2, 15),
    AVX512F              = feature(2, 16),
    AVX512DQ             = feature(2, 17),
    RDSEED               = feature(2, 18),
    ADX                  = feature(2, 19),
    SMAP                 = feature(2, 20),
    AVX512_IFMA          = feature(2, 21),
    CLFLUSHOPT           = feature(2, 23),
    CLWB                 = feature(2, 24),
    PROCESSOR_TRACE      = feature(2, 25),
    AVX512PF             = feature(2, 26),
    AVX512ER             = feature(2, 27),
    AVX512CD             = feature(2, 28),
    SHA                  = feature(2, 29),
    AVX512BW             = feature(2, 30),
    AVX512VL             = feature(2, 31),

    // 07H.0.ECX
    PREFETCHWT1      = feature(3, 0),
    AVX512_VBMI      = feature(3, 1),
    UMIP             = feature(3, 2),
    PKU              = feature(3, 3),
    OSPKE            = feature(3, 4),
    WAITPKG          = feature(3, 5),
    AVX512_VBMI2     = feature(3, 6),
    CET_SS           = feature(3, 7),
    GFNI             = feature(3, 8),
    VAES             = feature(3, 9),
    VPCLMULQDQ       = feature(3, 10),
    AVX512_VNNI      = feature(3, 11),
    AVX512_BITALG    = feature(3, 12),
    TME_EN           = feature(3, 13),
    AVX512_VPOPCNTDQ = feature(3, 14),
    LA57             = feature(3, 16),
    RDPID            = feature(3, 22),
    KL               = feature(3, 23),
    BUS_LOCK_DETECT  = feature(3, 24),
    CLDEMOTE         = feature(3, 25),
    MOVDIRI          = feature(3, 27),
    MOVDIR64B        = feature(3, 28),
    ENQCMD           = feature(3, 29),
    SGX_LC           = feature(3, 30),
    PKS              = feature(3, 31),

    // 07H.0.EDX
    AVX512_4VNNIW       = feature(4, 2),
    AVX512_4FMAPS       = feature(4, 3),
    FSRM                = feature(4, 4),
    AVX512_VP2INTERSECT = feature(4, 8),
    SRBDS_CTRL          = feature(4, 9),
    MD_CLEAR            = feature(4, 10),
    RTM_ALWAYS_ABORT    = feature(4, 11),
    RTM_FORCE_ABORT     = feature(4, 13),
    SERIALIZE           = feature(4, 14),
    HYBRID              = feature(4, 15),
    TSXLDTRK            = feature(4, 16),
    PCONFIG             = feature(4, 18),
    ARCH_LBR            = feature(4, 19),
    CET_IBT             = feature(4, 20),
    AMX_BF16            = feature(4, 22),
    AVX512_FP16         = feature(4, 23),
    AMX_TILE            = feature(4, 24),
    AMX_INT8            = feature(4, 25),
    SPEC_CTRL           = feature(4, 26),
    INTEL_STIBP         = feature(4, 27),
    L1D_FLUSH           = feature(4, 28),
    ARCH_CAPABILITIES   = feature(4, 29),
    CORE_CAPABILITIES   = feature(4, 30),
    SSBD                = feature(4, 31),

    // 80000001H.EDX
    SYSCALL  = feature(5, 11),
    NX       = feature(5, 20),
    MMXEXT   = feature(5, 22),
    FXSR_OPT = feature(5, 25),
    PDPE1GB  = feature(5, 26),
    RDTSCP   = feature(5, 27),
    LM       = feature(5, 29),

    // 80000001H.ECX
    LAHF_LM      = feature(6, 0),
    SVM          = feature(6, 2),
    EXTAPIC      = feature(6, 3),
    CR8_LEGACY   = feature(6, 4),
    LZCNT        = feature(6, 5),
    SSE4A        = feature(6, 6),
    MISALIGNSSE  = feature(6, 7),
    PREFETCHW    = feature(6, 8),
    OSVW         = feature(6, 9),
    IBS          = feature(6, 10),
    XOP          = feature(6, 11),
    SKINIT       = feature(6, 12),
    WDT          = feature(6, 13),
    LWP          = feature(6, 15),
    FMA4         = feature(6, 16),
    TBM          = feature(6, 21),
    TOPOEXT      = feature(6, 22),
    PERFCTR_CORE = feature(6, 23),
    PERFCTR_NB   = feature(6, 24),
};

CPUID cpuid(u32 leaf);

StringView feature_to_name(Feature);

class CPUFeatures {
public:
    constexpr bool has(Feature f) const {
        u32 i = static_cast<u32>(f);
        return (m_words[i / 32] >> (i % 32)) & 1;
    }

    constexpr void set(Feature f) {
        u32 i = static_cast<u32>(f);
        m_words[i / 32] |= (1u << (i % 32));
    }

    constexpr void clear(Feature f) {
        u32 i = static_cast<u32>(f);
        m_words[i / 32] &= ~(1u << (i % 32));
    }

    template<typename F>
    void for_each(F&& func) const {
        for (size_t word = 0; word < WORD_COUNT; word++) {
            u32 bits = m_words[word];

            // This will call `func` with features that don't exist so the caller has to
            // make sure that the feature is an actual feature.
            for (size_t bit = 0; bit < 32; bit++) {
                if (!(bits & (1 << bit))) {
                    continue;
                }

                func(static_cast<Feature>(word * 32 + bit));
            }
        }
    }

    static CPUFeatures detect();

private:
    u32 m_words[WORD_COUNT] = {};
};


}
