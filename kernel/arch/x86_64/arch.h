#pragma once

#include <kernel/common.h>

namespace kernel::arch {

static constexpr u32 KERNEL_CODE_SELECTOR = 0x28;
static constexpr u32 KERNEL_DATA_SELECTOR = 0x30;

static constexpr u32 USER_DATA_SELECTOR = 0x38;
static constexpr u32 USER_CODE_SELECTOR = 0x40; 

}
    