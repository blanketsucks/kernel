#pragma once

#include <kernel/common.h>
#include <kernel/time/timer.h>

namespace kernel {

class APICTimer : public Timer {
public:
    static constexpr u32 DEFAULT_FREQUENCY = 1'000;

    static RefPtr<APICTimer> create(u8 irq);

    u32 frequency() const { return m_frequency; }

    void eoi() override;

    void enable_local_timer(u32 frequency = DEFAULT_FREQUENCY);

private:
    APICTimer(u8 irq);

    void calibrate();

    u32 m_frequency;
};

}
