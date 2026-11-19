#include <kernel/time/hpet/timer.h>
#include <kernel/time/manager.h>
#include <kernel/time/hpet/hpet.h>
#include <kernel/time/rtc.h>
#include <kernel/time/pit.h>
#include <kernel/process/scheduler.h>
#include <kernel/acpi/acpi.h>
#include <kernel/arch/interrupts/apic.h>
#include <kernel/process/wait_queue.h>

#include <std/asserts.h>

namespace kernel {

static TimeManager* s_instance = nullptr;

void TimeManager::initialize() {
    if (!s_instance) {
        s_instance = new TimeManager();
    }

    s_instance->init();
}

TimeManager* TimeManager::instance() {
    return s_instance;
}

void TimeManager::init() {
    rtc::init();
    m_epoch_time = Duration::from_seconds(rtc::boot_time());

    if (APIC::is_initialized()) {
        auto& apic = APIC::instance();
        auto timer = apic.create_timer();

        this->set_event_timer(move(timer));
        if (HPET::initialize()) {
            auto* hpet = HPET::instance();
            this->set_clock_source(hpet->clock_source());
        }
    } else {
        // Fall back to the PIT if the HPET is not available
        if (!HPET::initialize()) {
            this->initialize_with_pit();
        } else {
            this->initialize_with_hpet();
        }
    }

    ASSERT_MSG(m_clock_source, "No clock source found on this system.");

    dbgln("Time Manager:");
    dbgln(" - Event timer: {}", m_event_timer->name());
    dbgln(" - Clock source: {}", m_clock_source->name());
    dbgln();

    m_event_timer->start_periodic(DEFAULT_FREQUENCY);
}

void TimeManager::initialize_with_hpet() {
    auto* hpet = HPET::instance();

    this->set_clock_source(hpet->clock_source());
    this->set_event_timer(hpet->timer(0));
}

void TimeManager::initialize_with_pit() {
    this->set_event_timer(PIT::create());
    // TODO: Use a clock source for the PIT
}

void TimeManager::add_waiter(WaitEntry& entry) {
    auto* tm = s_instance;
    ASSERT(tm);

    tm->m_waiters.with([&](ClockList& clocks) {
        auto& list = clocks[entry.m_deadline.clock_id()];
        for (auto& other : list) {
            if (other.m_deadline.at() > entry.m_deadline.at()) {
                list.insert_before(other, entry);
                return;
            }
        }

        list.append(entry);
    });
}

void TimeManager::cancel_waiter(WaitEntry& entry) {
    auto* tm = s_instance;
    ASSERT(tm);

    tm->m_waiters.with([&](ClockList& clocks) {
        auto& list = clocks[entry.m_deadline.clock_id()];
        list.remove(entry);
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
    auto& processor = Processor::current();
    if (processor.cpu() == 0) {
        this->update_time();
    }

    processor.invoke_scheduler_async();
}

void TimeManager::update_time() {
    u64 now = m_clock_source->read();
    u64 cycles = (now - m_last_counter) & m_clock_source->mask();

    auto [ns, frac] = cycles_to_ns(cycles, m_ns_frac);
    u64 total_ns = static_cast<u64>(m_ns) + ns;

    u32 iteration = m_timer_update2.fetch_add(1, std::MemoryOrder::Acquire);
    {
        m_last_counter = now;
        m_ns_frac      = frac;

        m_seconds_since_boot += total_ns / 1'000'000'000ull;
        m_ns = static_cast<u32>(total_ns % 1'000'000'000ull);

        m_epoch_time += Duration::from_nanoseconds(ns);
    }
    m_timer_update1.store(iteration + 1, std::MemoryOrder::Release);

    this->wakeup_waiters();
}

void TimeManager::wakeup_waiters() {
    m_waiters.with([](ClockList& clocks) {
        for (auto& list : clocks) {
            while (!list.empty()) {
                auto& entry = list.first();
                if (!entry.m_deadline.is_expired()) {
                    break;
                }

                entry.m_queue->on_timeout(entry);
                list.remove(entry);
            }
        }
    });
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
    u64 seconds, last_counter, frac;
    u32 iteration, ns;

    do {
        iteration = m_timer_update1.load(std::MemoryOrder::Acquire);

        seconds = m_seconds_since_boot;
        ns = m_ns;
        last_counter = m_last_counter;
        frac = m_ns_frac;
    } while (iteration != m_timer_update2.load(std::MemoryOrder::Acquire));

    u64 cycles = (m_clock_source->read() - last_counter) & m_clock_source->mask();
    u64 total_ns = static_cast<u64>(ns) + cycles_to_ns(cycles, frac).ns;

    return Duration(
        seconds + total_ns / 1'000'000'000ull,
        static_cast<u32>(total_ns % 1'000'000'000ull)
    );
}

auto TimeManager::cycles_to_ns(u64 cycles, u64 frac) -> NSConv {
    u64 frequency = m_clock_source->frequency();

    u64 secs = cycles / frequency;
    u64 rem  = cycles % frequency;

    u64 acc = rem  * 1'000'000'000 + frac;
    u64 ns  = secs * 1'000'000'000 + (acc / frequency);

    return { ns, acc % frequency };
}

}
