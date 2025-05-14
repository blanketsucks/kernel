#include <kernel/time/pit.h>
#include <kernel/arch/io.h>
#include <kernel/time/manager.h>

#include <std/format.h>

namespace kernel {

static PIT* s_pit = nullptr;

void PIT::init() {
    if (s_pit) {
        return;
    }

    s_pit = new PIT();

    s_pit->set_frequency(DEFAULT_FREQUENCY);
    s_pit->enable_irq();
}

PIT* PIT::instance() {
    return s_pit;
}

void PIT::handle_irq() {
    m_ticks++;
    TimeManager::tick();
}

void PIT::set_frequency(u32 frequency) {
    u32 divisor = INPUT_CLOCK / frequency;

    u8 low = divisor & 0xFF;
    u8 high = (divisor >> 8) & 0xFF;

    io::write<u8>(COMMAND, 0x36);

    io::write<u8>(CHANNEL0, low);
    io::write<u8>(CHANNEL0, high);
}

}