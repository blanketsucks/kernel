#include <kernel/arch/cpuid.h>
#include <cpuid.h>

namespace kernel::arch {

CPUID cpuid(u32 leaf) {
    CPUID r {};
    __get_cpuid(leaf, &r.eax, &r.ebx, &r.ecx, &r.edx);

    return r;
}

CPUFeatures CPUFeatures::detect() {
    CPUFeatures features;

    u32 max_extended_leaf = cpuid(EXTENDED_LEAF).eax;
    for (size_t i = 0; i < WORD_COUNT; i++) {
        const auto& w = CPUID_WORDS[i];
        if (w.leaf > max_extended_leaf) {
            continue;
        }

        auto r = cpuid(w.leaf);
        u32 value = 0;

        switch (w.reg) {
            case CPUIDReg::eax: value = r.eax; break;
            case CPUIDReg::ebx: value = r.ebx; break;
            case CPUIDReg::ecx: value = r.ecx; break;
            case CPUIDReg::edx: value = r.edx; break;
        }

        features.m_words[i] = value;
    }

    return features;
}

StringView feature_to_name(Feature f) {
    switch (f) {
        // 01H.EDX
        case Feature::FPU:   return "fpu";
        case Feature::VME:   return "vme";
        case Feature::DE:    return "de";
        case Feature::PSE:   return "pse";
        case Feature::TSC:   return "tsc";
        case Feature::MSR:   return "msr";
        case Feature::PAE:   return "pae";
        case Feature::MCE:   return "mce";
        case Feature::CX8:   return "cx8";
        case Feature::APIC:  return "apic";
        case Feature::SEP:   return "sep";
        case Feature::MTRR:  return "mtrr";
        case Feature::PGE:   return "pge";
        case Feature::MCA:   return "mca";
        case Feature::CMOV:  return "cmov";
        case Feature::PAT:   return "pat";
        case Feature::PSE36: return "pse36";
        case Feature::PSN:   return "psn";
        case Feature::CLFSH: return "clfsh";
        case Feature::DS:    return "ds";
        case Feature::ACPI:  return "acpi";
        case Feature::MMX:   return "mmx";
        case Feature::FXSR:  return "fxsr";
        case Feature::SSE:   return "sse";
        case Feature::SSE2:  return "sse2";
        case Feature::SS:    return "ss";
        case Feature::HTT:   return "htt";
        case Feature::TM:    return "tm";
        case Feature::PBE:   return "pbe";

        // 01H.ECX
        case Feature::SSE3:         return "sse3";
        case Feature::PCLMULQDQ:    return "pclmulqdq";
        case Feature::DTES64:       return "dtes64";
        case Feature::MONITOR:      return "monitor";
        case Feature::DS_CPL:       return "ds_cpl";
        case Feature::VMX:          return "vmx";
        case Feature::SMX:          return "smx";
        case Feature::EIST:         return "eist";
        case Feature::TM2:          return "tm2";
        case Feature::SSSE3:        return "ssse3";
        case Feature::CNXT_ID:      return "cnxt_id";
        case Feature::SDBG:         return "sdbg";
        case Feature::FMA:          return "fma";
        case Feature::CMPXCHG16B:   return "cmpxchg16b";
        case Feature::XTPR:         return "xtpr";
        case Feature::PDCM:         return "pdcm";
        case Feature::PCID:         return "pcid";
        case Feature::DCA:          return "dca";
        case Feature::SSE4_1:       return "sse4_1";
        case Feature::SSE4_2:       return "sse4_2";
        case Feature::X2APIC:       return "x2apic";
        case Feature::MOVBE:        return "movbe";
        case Feature::POPCNT:       return "popcnt";
        case Feature::TSC_DEADLINE: return "tsc_deadline";
        case Feature::AESNI:        return "aesni";
        case Feature::XSAVE:        return "xsave";
        case Feature::OSXSAVE:      return "osxsave";
        case Feature::AVX:          return "avx";
        case Feature::F16C:         return "f16c";
        case Feature::RDRAND:       return "rdrand";
        case Feature::HYPERVISOR:   return "hypervisor";

        // 07H.EBX
        case Feature::FSGSBASE:             return "fsgsbase";
        case Feature::TSC_ADJUST:           return "tsc_adjust";
        case Feature::SGX:                  return "sgx";
        case Feature::BMI1:                 return "bmi1";
        case Feature::HLE:                  return "hle";
        case Feature::AVX2:                 return "avx2";
        case Feature::FDP_EXCPTN_ONLY:      return "fdp_excptn_only";
        case Feature::SMEP:                 return "smep";
        case Feature::BMI2:                 return "bmi2";
        case Feature::ERMS:                 return "erms";
        case Feature::INVPCID:              return "invpcid";
        case Feature::RTM:                  return "rtm";
        case Feature::RDT_M:                return "rdt_m";
        case Feature::FPU_CS_DS_DEPRECATED: return "fpu_cs_ds_deprecated";
        case Feature::MPX:                  return "mpx";
        case Feature::RDT_A:                return "rdt_a";
        case Feature::AVX512F:              return "avx512f";
        case Feature::AVX512DQ:             return "avx512dq";
        case Feature::RDSEED:               return "rdseed";
        case Feature::ADX:                  return "adx";
        case Feature::SMAP:                 return "smap";
        case Feature::AVX512_IFMA:          return "avx512_ifma";
        case Feature::CLFLUSHOPT:           return "clflushopt";
        case Feature::CLWB:                 return "clwb";
        case Feature::PROCESSOR_TRACE:      return "processor_trace";
        case Feature::AVX512PF:             return "avx512pf";
        case Feature::AVX512ER:             return "avx512er";
        case Feature::AVX512CD:             return "avx512cd";
        case Feature::SHA:                  return "sha";
        case Feature::AVX512BW:             return "avx512bw";
        case Feature::AVX512VL:             return "avx512vl";

        // 07H.ECX
        case Feature::PREFETCHWT1:      return "prefetchwt1";
        case Feature::AVX512_VBMI:      return "avx512_vbmi";
        case Feature::UMIP:             return "umip";
        case Feature::PKU:              return "pku";
        case Feature::OSPKE:            return "ospke";
        case Feature::WAITPKG:          return "waitpkg";
        case Feature::AVX512_VBMI2:     return "avx512_vbmi2";
        case Feature::CET_SS:           return "cet_ss";
        case Feature::GFNI:             return "gfni";
        case Feature::VAES:             return "vaes";
        case Feature::VPCLMULQDQ:       return "vpclmulqdq";
        case Feature::AVX512_VNNI:      return "avx512_vnni";
        case Feature::AVX512_BITALG:    return "avx512_bitalg";
        case Feature::TME_EN:           return "tme_en";
        case Feature::AVX512_VPOPCNTDQ: return "avx512_vpopcntdq";
        case Feature::LA57:             return "la57";
        case Feature::RDPID:            return "rdpid";
        case Feature::KL:               return "kl";
        case Feature::BUS_LOCK_DETECT:  return "bus_lock_detect";
        case Feature::CLDEMOTE:         return "cldemote";
        case Feature::MOVDIRI:          return "movdiri";
        case Feature::MOVDIR64B:        return "movdir64b";
        case Feature::ENQCMD:           return "enqcmd";
        case Feature::SGX_LC:           return "sgx_lc";
        case Feature::PKS:              return "pks";

        // 07H.EDX
        case Feature::AVX512_4VNNIW:       return "avx512_4vnniw";
        case Feature::AVX512_4FMAPS:       return "avx512_4fmaps";
        case Feature::FSRM:                return "fsrm";
        case Feature::AVX512_VP2INTERSECT: return "avx512_vp2intersect";
        case Feature::SRBDS_CTRL:          return "srbds_ctrl";
        case Feature::MD_CLEAR:            return "md_clear";
        case Feature::RTM_ALWAYS_ABORT:    return "rtm_always_abort";
        case Feature::RTM_FORCE_ABORT:     return "rtm_force_abort";
        case Feature::SERIALIZE:           return "serialize";
        case Feature::HYBRID:              return "hybrid";
        case Feature::TSXLDTRK:            return "tsxldtrk";
        case Feature::PCONFIG:             return "pconfig";
        case Feature::ARCH_LBR:            return "arch_lbr";
        case Feature::CET_IBT:             return "cet_ibt";
        case Feature::AMX_BF16:            return "amx_bf16";
        case Feature::AVX512_FP16:         return "avx512_fp16";
        case Feature::AMX_TILE:            return "amx_tile";
        case Feature::AMX_INT8:            return "amx_int8";
        case Feature::SPEC_CTRL:           return "spec_ctrl";
        case Feature::INTEL_STIBP:         return "intel_stibp";
        case Feature::L1D_FLUSH:           return "l1d_flush";
        case Feature::ARCH_CAPABILITIES:   return "arch_capabilities";
        case Feature::CORE_CAPABILITIES:   return "core_capabilities";
        case Feature::SSBD:                return "ssbd";

        // 80000001H.EDX
        case Feature::SYSCALL:  return "syscall";
        case Feature::NX:       return "nx";
        case Feature::MMXEXT:   return "mmxext";
        case Feature::FXSR_OPT: return "fxsr_opt";
        case Feature::PDPE1GB:  return "pdpe1gb";
        case Feature::RDTSCP:   return "rdtscp";
        case Feature::LM:       return "lm";

        // 80000001H.ECX
        case Feature::LAHF_LM:      return "lahf_lm";
        case Feature::SVM:          return "svm";
        case Feature::EXTAPIC:      return "extapic";
        case Feature::CR8_LEGACY:   return "cr8_legacy";
        case Feature::LZCNT:        return "lzcnt";
        case Feature::SSE4A:        return "sse4a";
        case Feature::MISALIGNSSE:  return "misalignsse";
        case Feature::PREFETCHW:    return "prefetchw";
        case Feature::OSVW:         return "osvw";
        case Feature::IBS:          return "ibs";
        case Feature::XOP:          return "xop";
        case Feature::SKINIT:       return "skinit";
        case Feature::WDT:          return "wdt";
        case Feature::LWP:          return "lwp";
        case Feature::FMA4:         return "fma4";
        case Feature::TBM:          return "tbm";
        case Feature::TOPOEXT:      return "topoext";
        case Feature::PERFCTR_CORE: return "perfctr_core";
        case Feature::PERFCTR_NB:   return "perfctr_nb";
    }

    return "unknown";
}


}
