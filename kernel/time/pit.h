#pragma once

#include <kernel/arch/irq.h>
#include <kernel/time/event_timer.h>

#include <std/memory.h>

namespace kernel {

class PIT : public EventTimer, IRQHandler {
public:
    enum Access {
        Countdown     = 0x0,
        OneShot       = 0x1,
        RateGenerator = 0x2,
        SquareWave    = 0x3,

        Channel0 = 0x00,
        Channel1 = 0x40,
        Channel2 = 0x80,

        LatchCount  = 0x00,
        LowByte     = 0x10,
        HighByte    = 0x20,
        LowHighByte = 0x30,
    };

    static constexpr u32 CHANNEL0 = 0x40;
    static constexpr u32 CHANNEL1 = 0x41;
    static constexpr u32 CHANNEL2 = 0x42;
    static constexpr u32 COMMAND  = 0x43;

    static constexpr u32 INPUT_CLOCK = 1'193'182;

    static RefPtr<PIT> create() {
        return RefPtr(new PIT);
    }

    static void wait_ms(u32 ms);

    StringView name() const override { return "PIT"; }
    u64 frequency() const override { return INPUT_CLOCK; };

    bool supports_oneshot() const override { return false; }
    bool supports_periodic() const override { return true; }

    void start_periodic(u64 hz) override;
    void start_oneshot(u64) override {};

    void stop() override;

    void handle_irq() override { dispatch(); }

private:
    PIT();
};

}
