#pragma once

#include <kernel/cpu/pic.h>

namespace kernel::pit {

static constexpr u32 INPUT_CLOCK = 1193182; // 1.193182 MHz

static constexpr u32 CHANNEL0 = 0x40;
static constexpr u32 COMMAND = 0x43;

static constexpr u32 DEFAULT_FREQUENCY = 100;

void init();
void set_frequency(u32 frequency);

u32 ticks();

}