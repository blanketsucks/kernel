#include <kernel/time/pit.h>
#include <kernel/io.h>
#include <kernel/serial.h>
#include <kernel/arch/cpu.h>
#include <kernel/process/scheduler.h>

#include <std/format.h>

namespace kernel::pit {

u32 s_ticks = 0;
PIT* s_pit = nullptr;

void PIT::handle_interrupt(arch::InterruptRegisters*) {
    pic::eoi(0);
    s_ticks++;

    Scheduler::invoke_async();
}

void init() {
    set_frequency(DEFAULT_FREQUENCY);
    // s_pit = new PIT();
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