#pragma once

#include <kernel/arch/irq.h>
#include <kernel/time/timer.h>

#include <std/memory.h>

namespace kernel {

class PIT : public Timer {
public:
    enum Access {
        Countdown = 0x0,
        OneShot = 0x1,
        RateGenerator = 0x2,
        SquareWave = 0x3,

        Channel0 = 0x00,
        Channel1 = 0x40,
        Channel2 = 0x80,

        LatchCount = 0x00,
        LowByte = 0x10,
        HighByte = 0x20,
        LowHighByte = 0x30,
    };

    static constexpr u32 INPUT_CLOCK = 1193182; // 1.193182 MHz

    static constexpr u32 CHANNEL0 = 0x40;
    static constexpr u32 CHANNEL1 = 0x41;
    static constexpr u32 CHANNEL2 = 0x42;
    static constexpr u32 COMMAND = 0x43;

    static constexpr u32 DEFAULT_FREQUENCY = 100;

    static RefPtr<PIT> create();

    size_t ticks() const { return m_ticks; }
    size_t frequency() const { return m_frequency; }
    
    void set_frequency(u32 frequency);

private:
    PIT();

    size_t m_ticks = 0;
    u32 m_frequency = DEFAULT_FREQUENCY;
};

void init();
void set_frequency(u32 frequency);

u32 ticks();

}