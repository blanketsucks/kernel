#include <kernel/time/manager.h>
#include <kernel/time/hpet/hpet.h>
#include <kernel/time/rtc.h>
#include <kernel/time/pit.h>
#include <kernel/process/scheduler.h>

namespace kernel {

static TimeManager* s_instance = nullptr;

void TimeManager::init() {
    if (s_instance) {
        return;
    }

    s_instance = new TimeManager();
    s_instance->initialize();
}

TimeManager* TimeManager::instance() {
    return s_instance;
}

void TimeManager::initialize() {
    rtc::init();

    m_epoch_time = rtc::boot_time() * 1e9;

    // Fall back to the PIT if the HPET is not available
    if (!HPET::init()) {
        this->initialize_with_pit();
    } else {
        this->initialize_with_hpet();
    }
}

void TimeManager::initialize_with_hpet() {
    auto* hpet = HPET::instance();
    m_ticks_per_second = hpet->frequency();

    m_system_timer = hpet->timer(0);
    m_system_timer->set_callback([this]() {
        this->timer_tick();
    });
}

void TimeManager::initialize_with_pit() {
    m_ticks_per_second = PIT::DEFAULT_FREQUENCY;

    m_system_timer = PIT::create();
    m_system_timer->set_callback([this]() {
        this->timer_tick();
    });
}

time_t TimeManager::boot_time() {
    return rtc::boot_time();
}

void TimeManager::timer_tick() {
    this->update_time();
    Scheduler::invoke_async();
}

void TimeManager::update_time() {
    if (!HPET::instance()) {
        return;
    }

    auto* hpet = HPET::instance();
    u64 delta = hpet->deltatime_ns(m_seconds_since_boot, m_ticks);

    m_epoch_time += delta;
}

u64 TimeManager::epoch_time() {
    ScopedSpinLock lock(m_lock);
    return m_epoch_time;
}

u64 TimeManager::monotonic_time() {
    ScopedSpinLock lock(m_lock);
    if (!HPET::instance()) {
        return 0;
    }

    u64 ticks = m_ticks;
    u64 seconds = m_seconds_since_boot;

    auto* hpet = HPET::instance();
    hpet->deltatime_ns(seconds, ticks, false);

    u64 ns = (ticks * 1e9) / m_ticks_per_second;
    return ns + (seconds * 1e9);
}

}