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
    
    // Fall back to the PIT if the HPET is not available
    if (!HPET::init()) {
        PIT::init();
    }
}

time_t TimeManager::boot_time() {
    return rtc::boot_time();
}

void TimeManager::tick() {
    s_instance->timer_tick();
}

void TimeManager::timer_tick() {
    // TODO: Update the system time

    Scheduler::invoke_async();
}

}