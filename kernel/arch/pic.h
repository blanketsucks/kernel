#pragma once

#include <kernel/common.h>
#include <kernel/arch/registers.h>

namespace kernel::pic {

constexpr u8 MASTER_COMMAND = 0x20;
constexpr u8 MASTER_DATA = 0x21;

constexpr u8 SLAVE_COMMAND = 0xA0;
constexpr u8 SLAVE_DATA = 0xA1;

constexpr u8 EOF = 0x20;

void disable();
void remap();

void init();

void eoi(u8 irq);

void disable(u8 irq);
void enable(u8 irq);


}