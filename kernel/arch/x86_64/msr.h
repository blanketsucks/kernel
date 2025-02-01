#pragma once

#include <kernel/common.h>

namespace kernel::arch {

enum {
    MSR_EFER    = 0xC0000080,
    MSR_STAR    = 0xC0000081,
    MSR_LSTAR   = 0xC0000082,
    MSR_CSTAR   = 0xC0000083,
    MSR_SFMASK  = 0xC0000084,
    MSR_FS_BASE = 0xC0000100,
    MSR_GS_BASE = 0xC0000101,
};

void wmsr(u32 msr, u64 value);
u64 rmsr(u32 msr);

}