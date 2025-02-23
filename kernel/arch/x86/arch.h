#pragma once

#include <kernel/common.h>

namespace kernel::arch {

static constexpr u32 KERNEL_CODE_SELECTOR = 0x08;
static constexpr u32 KERNEL_DATA_SELECTOR = 0x10;

static constexpr u32 USER_CODE_SELECTOR = 0x18;
static constexpr u32 USER_DATA_SELECTOR = 0x20;

}