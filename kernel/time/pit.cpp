#include <kernel/time/pit.h>
#include <kernel/io.h>
#include <kernel/serial.h>

namespace kernel::pit {

u32 s_ticks = 0;

INTERRUPT static void _irq0_handler(cpu::InterruptFrame*) {
    s_ticks++;
    pic::eoi(0);
}

void init() {
    pic::set_irq_handler(0, _irq0_handler);
    set_frequency(DEFAULT_FREQUENCY);
}

void set_frequency(u32 frequency) {
    u32 divisor = INPUT_CLOCK / frequency;

    u8 low = divisor & 0xFF;
    u8 high = (divisor >> 8) & 0xFF;

    io::write<u8>(COMMAND, 0x36);

    io::write<u8>(CHANNEL0, low);
    io::write<u8>(CHANNEL0, high);
}

u32 ticks() {
    return s_ticks;
}

}