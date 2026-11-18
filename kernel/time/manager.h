#pragma once

#include <kernel/common.h>
#include <kernel/posix/sys/types.h>
#include <kernel/process/wait_queue.h>
#include <kernel/posix/time.h>
#include <kernel/time/timer.h>
#include <kernel/sync/spinlock.h>
#include <kernel/sync/resource.h>

#include <std/atomic.h>
#include <std/time.h>
#include <std/memory.h>
#include <std/array.h>

namespace kernel {

class TimeManager {
public:
    static void initialize();
    static TimeManager* instance();

    static timespec ns_to_timespec(u64 ns) {
        return { static_cast<time_t>(ns / 1'000'000'000), static_cast<long>(ns % 1'000'000'000) };
    }

    static Duration query_time(clockid_t);

    static void add_waiter(WaitEntry&);
    static void cancel_waiter(WaitEntry&);

    static time_t boot_time();
    static void tick();

    Duration epoch_time();
    Duration monotonic_time();

    void set_system_timer(RefPtr<TimerBase> timer);

private:
    using WaiterList = std::IntrusiveList<&WaitEntry::m_timer_node>;
    using ClockList = std::Array<WaiterList, CLOCK_COUNT>;

    TimeManager() = default;

    void init();

    void initialize_with_pit();
    void initialize_with_hpet();

    void timer_tick();
    void update_time();

    void wakeup_waiters();

    RefPtr<TimerBase> m_system_timer;
    RefPtr<TimerBase> m_time_keep_timer;

    u64 m_ticks_per_second = 0;

    u64 m_seconds_since_boot = 0;
    u64 m_ticks = 0;

    Duration m_epoch_time;

    SpinLockResource<ClockList> m_waiters;

    // TODO: Maybe use a lock instead in epoch_time() and monotonic_time(), but for some reason adding a lock
    // makes the kernel go insane and crash.
    std::Atomic<u32> m_timer_update1;
    std::Atomic<u32> m_timer_update2;
};

}
