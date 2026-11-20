#pragma once

#include <kernel/common.h>
#include <kernel/posix/sys/types.h>
#include <kernel/process/wait_queue.h>
#include <kernel/posix/time.h>
#include <kernel/time/clock_source.h>
#include <kernel/time/event_timer.h>
#include <kernel/sync/spinlock.h>
#include <kernel/sync/resource.h>

#include <std/atomic.h>
#include <std/time.h>
#include <std/memory.h>
#include <std/array.h>
#include <std/vector.h>

namespace kernel {

class TimeManager {
public:
    static constexpr u64 DEFAULT_FREQUENCY = 1'000;

    static void initialize();
    static TimeManager* instance();

    static timespec ns_to_timespec(u64 ns) {
        return { static_cast<time_t>(ns / 1'000'000'000), static_cast<long>(ns % 1'000'000'000) };
    }

    static Duration query_time(clockid_t);

    static void add_waiter(WaitEntry&);
    static void cancel_waiter(WaitEntry&);

    static time_t boot_time();

    Duration epoch_time();
    Duration monotonic_time();

private:
    struct NSConv {
        u64 ns;
        u64 frac;
    };

    using WaiterList = std::IntrusiveList<&WaitEntry::m_timer_node>;
    using ClockList = std::Array<WaiterList, CLOCK_COUNT>;

    TimeManager() = default;

    void init();

    void discover();

    void pick_best_event_timer();
    void pick_best_clock_source();

    void set_clock_source(RefPtr<ClockSource> clock_source) {
        m_clock_source = move(clock_source);
    }

    void set_event_timer(RefPtr<EventTimer> event_timer) {
        m_event_timer = move(event_timer);
        m_event_timer->set_callback([this]() {
            this->timer_tick();
        });
    }

    void timer_tick();
    void update_time();

    void wakeup_waiters();

    NSConv cycles_to_ns(u64 cycles, u64 frac);

    RefPtr<ClockSource> m_clock_source = nullptr;
    RefPtr<EventTimer>  m_event_timer  = nullptr;

    Vector<RefPtr<ClockSource>> m_clock_sources;
    Vector<RefPtr<EventTimer>>  m_event_timers;

    u64 m_seconds_since_boot = 0;
    u32 m_ns                 = 0;
    u64 m_last_counter       = 0;
    u64 m_ns_frac            = 0;

    Duration m_epoch_time;

    SpinLockResource<ClockList> m_waiters;

    std::Atomic<u32> m_timer_update1;
    std::Atomic<u32> m_timer_update2;
};

}
