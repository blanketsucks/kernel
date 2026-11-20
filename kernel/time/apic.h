#pragma once

#include <kernel/common.h>
#include <kernel/time/event_timer.h>
#include <kernel/arch/irq.h>

namespace kernel {

class APICTimer : public EventTimer, InterruptHandler {
public:
    static RefPtr<APICTimer> create(u8 irq);

    StringView name() const override { return "APIC"; }
    EventTimerType type() const override { return EventTimerType::LAPIC; }

    u64 frequency() const override { return m_frequency; }

    bool supports_oneshot() const override { return false; }
    bool supports_periodic() const override { return true; }

    void start_periodic(u64 hz) override;
    void start_oneshot(u64 ticks) override;

    void stop() override;

    void handle_interrupt() override { dispatch(); }
    void eoi() override;

    void enable_local_timer(u64 frequency);
    void disable_local_timer();

private:
    APICTimer(u8 irq);

    void calibrate();

    u32 m_frequency;
};

}
