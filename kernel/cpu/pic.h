#pragma once

#include <kernel/common.h>

namespace kernel::pic {

void disable();
void remap();

void send_eoi(u8 irq);

void disable(u8 irq);
void enable(u8 irq);

}