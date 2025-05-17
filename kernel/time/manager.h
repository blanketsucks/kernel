#pragma once

#include <kernel/common.h>
#include <kernel/posix/sys/types.h>
#include <kernel/posix/time.h>
#include <kernel/time/timer.h>
#include <kernel/sync/spinlock.h>

#include <std/atomic.h>
#include <std/time.h>

namespace kernel {

class TimeManager {
public:
    static void init();
    static TimeManager* instance();

    static timespec ns_to_timespec(u64 ns) {
        return { static_cast<time_t>(ns / 1'000'000'000), static_cast<long>(ns % 1'000'000'000) };
    }

    static Duration query_time(clockid_t);

    static time_t boot_time();
    static void tick();

    Duration epoch_time();
    Duration monotonic_time();

private:
    TimeManager() = default;

    void initialize();

    void initialize_with_pit();
    void initialize_with_hpet();

    void timer_tick();
    void update_time();

    RefPtr<Timer> m_system_timer;

    u64 m_ticks_per_second = 0;
    
    u64 m_seconds_since_boot = 0;
    u64 m_ticks = 0;

    Duration m_epoch_time;

    // TODO: Maybe use a lock instead in epoch_time() and monotonic_time(), but for some reason adding a lock
    // makes the kernel go insane and crash.
    std::Atomic<u32> m_timer_update1;
    std::Atomic<u32> m_timer_update2;
};

}