#include <kernel/time/manager.h>
#include <kernel/time/hpet/hpet.h>
#include <kernel/time/rtc.h>
#include <kernel/time/pit.h>
#include <kernel/process/scheduler.h>
#include <kernel/acpi/acpi.h>

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

    m_epoch_time = Duration::from_seconds(rtc::boot_time());

    auto* parser = acpi::Parser::instance();
    parser->init();

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

Duration TimeManager::query_time(clockid_t clock_id) {
    if (clock_id == CLOCK_REALTIME) {
        return s_instance->epoch_time();
    } else if (clock_id == CLOCK_MONOTONIC) {
        return s_instance->monotonic_time();
    }

    return Duration::from_nanoseconds(0);
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

    u64 seconds_since_boot = m_seconds_since_boot;
    u64 ticks = m_ticks;
    u64 delta = hpet->deltatime_ns(seconds_since_boot, ticks);

    u32 iteration = m_timer_update2.fetch_add(1, std::MemoryOrder::Acquire);

    m_ticks = ticks;
    m_seconds_since_boot = seconds_since_boot;

    m_epoch_time += Duration::from_nanoseconds(delta);

    m_timer_update1.store(iteration + 1, std::MemoryOrder::Release);
}

Duration TimeManager::epoch_time() {
    Duration time;
    u32 iteration;

    // We do this in order to avoid reading the time while it is being updated.
    // So we wait until the update is done before returning the time. The same applies to monotonic_time().
    // Took from SerenityOS. https://github.com/SerenityOS/serenity/blob/80f586503daf0ecd0eac858e8b8e7e53b43065ae/Kernel/Time/TimeManagement.cpp#L178
    do {
        iteration = m_timer_update1.load(std::MemoryOrder::Acquire);
        time = m_epoch_time;
    } while (iteration != m_timer_update2.load(std::MemoryOrder::Acquire));

    return time;
}

Duration TimeManager::monotonic_time() {
    if (!HPET::instance()) {
        return Duration::zero();
    }

    u32 ticks;
    u64 seconds;

    u32 iteration;
    do {
        iteration = m_timer_update1.load(std::MemoryOrder::Acquire);

        seconds = m_seconds_since_boot;
        ticks = m_ticks;
    } while (iteration != m_timer_update2.load(std::MemoryOrder::Acquire));

    u64 ns = ((u64)ticks * 1'000'000'000ull) / m_ticks_per_second;
    return Duration(seconds, static_cast<u32>(ns));
}

}