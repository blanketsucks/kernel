#pragma once

#include <kernel/common.h>
#include <kernel/time/hpet/defs.h>
#include <kernel/time/hpet/timer.h>
#include <kernel/time/clock_source.h>

#include <std/memory.h>

namespace kernel {

class HPETClockSource;

class HPET {
public:
    static bool initialize();
    static HPET* instance();

    HPETRegisters* registers() const { return m_registers; }
    u64 counter() const { return m_registers->main_counter_value; }

    u64 counter_clock_period() const { return m_counter_clock_period; } // In femtoseconds
    bool is_64_bit_counter() const { return m_is_64_bit_counter; }

    TimerConfiguration* timer_configuration_for(u8 id) const;

    RefPtr<HPETTimer> timer(u8 id) { return m_timers[id]; }
    RefPtr<HPETClockSource> clock_source() const { return m_clock_source; }

    void enable();
    void disable();

private:
    bool init();

    HPETRegisters* m_registers = nullptr;

    u32 m_counter_clock_period;
    u32 m_minimum_tick;

    bool m_is_64_bit_counter = false;

    u64 m_last_counter = 0;

    RefPtr<HPETClockSource> m_clock_source;
    Vector<RefPtr<HPETTimer>> m_timers;
};


class HPETClockSource : public ClockSource {
public:
    static RefPtr<HPETClockSource> create(HPET* hpet) {
        return RefPtr(new HPETClockSource(hpet));
    }

    StringView name() const override { return "HPET"; }

    u64 frequency() const override {
        return 1'000'000'000'000'000ull / m_hpet->counter_clock_period();
    }

    u64 read() const override { return m_hpet->counter(); }
    u64 mask() const override { return m_mask; }

private:
    HPETClockSource(HPET* hpet);

    HPET* m_hpet = nullptr;
    u64 m_mask = 0;
};

}
