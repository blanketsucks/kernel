#include <kernel/process/blocker.h>
#include <kernel/time/rtc.h>

namespace kernel {

bool SleepBlocker::should_unblock() {
    return m_wake_time <= rtc::now();
}



}