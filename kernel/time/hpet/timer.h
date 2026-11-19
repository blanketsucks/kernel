#pragma once

#include <kernel/common.h>
#include <kernel/time/hpet/defs.h>
#include <kernel/arch/irq.h>
#include <kernel/time/event_timer.h>

#include <std/memory.h>
#include <std/function.h>

namespace kernel {

class HPET;

static constexpr size_t DEFAULT_HPET_TIMER_FREQUENCY = 1'000;

class HPETTimer : public EventTimer, IRQHandler {
public:
    static RefPtr<HPETTimer> create(HPET*, u8 irq, u8 id);

    u8 id() const { return m_id; }

    StringView name() const override { return "HPET"; }
    u64 frequency() const override;

    bool supports_oneshot() const override { return true; }
    bool supports_periodic() const override { return true; }

    void start_periodic(u64 hz) override;
    void start_oneshot(u64 ticks) override;

    void stop() override;

    void handle_irq() override;
    void update();

    void enable();
    void disable();

private:
    HPETTimer(HPET*, u8 irq, u8 id);

    HPET* m_hpet;
    u8 m_id;

    TimerConfiguration* m_configuration;

    u64 m_target_frequency = 0;

    bool m_periodic_capable = false;
    bool m_is_periodic = false;
};

}
