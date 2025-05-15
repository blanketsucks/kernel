#pragma once

#include <kernel/common.h>
#include <kernel/time/hpet/defs.h>
#include <kernel/time/hpet/timer.h>

#include <std/memory.h>

namespace kernel {

class HPET {
public:
    static bool init();
    static HPET* instance();

    void enable();
    void disable();

    HPETRegisters* registers() const { return m_registers; }
    TimerConfiguration* timer_configuration_for(u8 id) const;

    RefPtr<HPETTimer> timer(u8 id) {
        return m_timers[id];
    }

    u32 counter_clock_period() const { return m_counter_clock_period; } // In femtoseconds
    u64 frequency() const { return 1'000'000'000'000'000ull / (u64)m_counter_clock_period; }

    u32 minimum_tick() const { return m_minimum_tick; }

    u64 counter();

    // Return the time passed in ns since the last call to this function
    u64 deltatime_ns(u64& seconds_since_boot, u64& current_ticks, bool update = true);

private:
    bool initialize();

    HPETRegisters* m_registers = nullptr;

    u32 m_counter_clock_period;
    u32 m_minimum_tick;

    u64 m_last_counter = 0;

    Vector<RefPtr<HPETTimer>> m_timers;
};


}