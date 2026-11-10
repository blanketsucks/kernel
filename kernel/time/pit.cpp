#include <kernel/time/pit.h>
#include <kernel/arch/io.h>
#include <kernel/time/manager.h>

#include <std/format.h>

#define CHANNEL2_GATE 0x61

namespace kernel {

RefPtr<PIT> PIT::create() {
    return RefPtr<PIT>(new PIT);
}

PIT::PIT() : Timer(0) {
    io::write<u8>(COMMAND, Access::Channel0 | Access::LowHighByte | Access::SquareWave);
    this->set_frequency(DEFAULT_FREQUENCY);

    this->enable_irq();
}

void PIT::set_frequency(u32 frequency) {
    u32 divisor = INPUT_CLOCK / frequency;

    u8 low = divisor & 0xFF;
    u8 high = (divisor >> 8) & 0xFF;

    io::write<u8>(CHANNEL0, low);
    io::write<u8>(CHANNEL0, high);

    m_frequency = frequency;
}

void PIT::wait_ms(u32 ms) {
    u16 count = static_cast<u16>(((u64)ms * INPUT_CLOCK + 500) / 1000);

    u8 gate = io::read<u8>(CHANNEL2_GATE);
    io::write<u8>(CHANNEL2_GATE, gate & ~0b00110001);

    u8 cmd = io::read<u8>(COMMAND);

    io::write<u8>(COMMAND, Access::Channel2 | Access::LowHighByte | Access::Countdown);
    io::write<u8>(CHANNEL2, count & 0xFF);
    io::write<u8>(CHANNEL2, count >> 8);

    io::write<u8>(CHANNEL2_GATE, (gate & ~0x02) | 0x01);
    while (!(io::read<u8>(CHANNEL2_GATE) & 0x20)) {}

    io::write<u8>(CHANNEL2_GATE, gate);
    io::write<u8>(COMMAND, cmd);
}

}
