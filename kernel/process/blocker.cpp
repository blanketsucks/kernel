#include <kernel/process/blocker.h>
#include <kernel/process/scheduler.h>
#include <kernel/process/threads.h>
#include <kernel/time/rtc.h>


namespace kernel {

void Blocker::wait() {
    auto* thread = Scheduler::current_thread();
    thread->block(this);
}

bool SleepBlocker::should_unblock() {
    return m_wake_time <= rtc::now();
}



}