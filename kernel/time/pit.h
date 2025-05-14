#pragma once

#include <kernel/arch/irq.h>

namespace kernel {

class PIT : public IRQHandler {
public:
    static constexpr u32 INPUT_CLOCK = 1193182; // 1.193182 MHz

    static constexpr u32 CHANNEL0 = 0x40;
    static constexpr u32 COMMAND = 0x43;

    static constexpr u32 DEFAULT_FREQUENCY = 100;

    static void init();
    static PIT* instance();

    size_t ticks() const { return m_ticks; }
    
    void set_frequency(u32 frequency);

    void handle_irq() override;

private:
    PIT() : IRQHandler(0) {}

    size_t m_ticks = 0;
};

void init();
void set_frequency(u32 frequency);

u32 ticks();

}