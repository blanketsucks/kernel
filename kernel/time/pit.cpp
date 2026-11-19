#include <kernel/time/pit.h>
#include <kernel/arch/io.h>
#include <kernel/time/manager.h>

#include <std/format.h>

#define CHANNEL2_GATE 0x61

namespace kernel {

PIT::PIT() : IRQHandler(0) {}

void PIT::start_periodic(u64 hz) {
    u32 divisor = frequency() / hz;

    u8 low = divisor & 0xFF;
    u8 high = (divisor >> 8) & 0xFF;

    io::write<u8>(PIT::CHANNEL0, low);
    io::write<u8>(PIT::CHANNEL0, high);

    this->enable_irq();
    io::write<u8>(COMMAND, Access::Channel0 | Access::LowHighByte | Access::SquareWave);
}

void PIT::stop() {
    if (!this->is_enabled()) {
        return;
    }

    // TODO: Properly disable channel 0
    this->disable_irq();
}

void PIT::wait_ms(u32 ms) {
    u16 count = static_cast<u16>(((u64)ms * INPUT_CLOCK + 500) / 1000);

    u8 gate = io::read<u8>(CHANNEL2_GATE);
    io::write<u8>(CHANNEL2_GATE, gate & ~0b00110001);

    io::write<u8>(COMMAND, Access::Channel2 | Access::LowHighByte | Access::Countdown);
    io::write<u8>(CHANNEL2, count & 0xFF);
    io::write<u8>(CHANNEL2, count >> 8);

    io::write<u8>(CHANNEL2_GATE, (gate & ~0x02) | 0x01);
    while (!(io::read<u8>(CHANNEL2_GATE) & 0x20)) {}

    io::write<u8>(CHANNEL2_GATE, gate);
}


}
