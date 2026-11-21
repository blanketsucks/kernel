#pragma once

#include <kernel/time/clock_source.h>
#include <kernel/arch/cpu.h>

#include <std/memory.h>
#include <std/format.h>

namespace kernel {

class TSCClockSource : public ClockSource {
public:
    static RefPtr<TSCClockSource> create() {
        return RefPtr(new TSCClockSource);
    }

    virtual StringView name() const { return "Invariant TSC"; };
    virtual ClockSourceType type() const { return ClockSourceType::TSC; }

    u64 frequency() const { return m_frequency; }

    u64 read() const { return arch::rdtsc(); };
    u64 mask() const { return ~0ull; }

private:
    TSCClockSource();

    u64 calibrate();

    u64 m_frequency = 0;
};

}
