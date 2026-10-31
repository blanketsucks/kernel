#pragma once

#include <kernel/common.h>

namespace kernel::arch {

static constexpr u64 KERNEL_CODE_SELECTOR = 0x28;
static constexpr u64 KERNEL_DATA_SELECTOR = 0x30;

static constexpr u64 USER_DATA_SELECTOR = 0x38;
static constexpr u64 USER_CODE_SELECTOR = 0x40;

static constexpr u64 TSS_SELECTOR = 0x48;

static_assert(KERNEL_DATA_SELECTOR + 16 == USER_CODE_SELECTOR, "Kernel DS + 16 must be equal to the User CS in order for SYSCALL to work properly.");
static_assert(KERNEL_DATA_SELECTOR +  8 == USER_DATA_SELECTOR, "Kernel DS + 8 must be equal to the User DS in order for SYSCALL to work properly.");

}
    