#include <kernel/time/pit.h>
#include <kernel/arch/io.h>
#include <kernel/time/manager.h>

#include <std/format.h>

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

}