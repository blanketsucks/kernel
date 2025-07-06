#pragma once

#include <kernel/common.h>
#include <kernel/time/hpet/defs.h>
#include <kernel/time/timer.h>
#include <kernel/arch/irq.h>

#include <std/memory.h>
#include <std/function.h>

namespace kernel {

class HPET;

static constexpr size_t DEFAULT_HPET_TIMER_FREQUENCY = 100;

class HPETTimer : public Timer {
public:
    static RefPtr<HPETTimer> create(HPET*, u8 irq, u8 id);

    u8 id() const { return m_id; }

    bool is_periodic_capable() const { return m_periodic_capable; }
    bool is_64_bit_capable() const { return m_64_bit_capable; }

    bool is_periodic() const { return m_is_periodic; }

    size_t frequency() const { return m_frequency; }
    bool set_frequency(size_t frequency);

    void handle_irq() override;
    void update();

    void enable();
    void disable();

private:
    HPETTimer(HPET*, u8 irq, u8 id);

    HPET* m_hpet;
    u8 m_id;

    TimerConfiguration* m_configuration;

    bool m_periodic_capable;
    bool m_64_bit_capable;

    bool m_is_periodic = false;
    size_t m_frequency = DEFAULT_HPET_TIMER_FREQUENCY;
};
    
}