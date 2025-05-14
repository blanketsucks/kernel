#pragma once

#include <kernel/common.h>
#include <kernel/posix/sys/types.h>
#include <kernel/posix/time.h>
#include <kernel/time/timer.h>
#include <kernel/sync/spinlock.h>

#include <std/atomic.h>

namespace kernel {

class TimeManager {
public:
    static void init();
    static TimeManager* instance();

    static timespec ns_to_timespec(u64 ns) {
        return { static_cast<time_t>(ns / 1e9), static_cast<long>(ns % (long)1e9) };
    }

    static time_t boot_time();
    static void tick();

    u64 epoch_time();
    u64 monotonic_time();

private:
    TimeManager() = default;

    void initialize();

    void initialize_with_pit();
    void initialize_with_hpet();

    void timer_tick();
    void update_time();

    u64 m_ticks_per_second = 0;
    
    u64 m_seconds_since_boot = 0;
    u64 m_ticks = 0;

    u64 m_epoch_time = 0; // in nanoseconds

    RefPtr<Timer> m_system_timer;

    SpinLock m_lock;
};

}