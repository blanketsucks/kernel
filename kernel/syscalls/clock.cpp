#include <kernel/process/process.h>
#include <kernel/process/threads.h>
#include <kernel/process/blocker.h>
#include <kernel/time/manager.h>

namespace kernel {

ErrorOr<FlatPtr> Process::sys$clock_gettime(clockid_t clock_id, timespec* ts) {
    if (clock_id < CLOCK_REALTIME || clock_id > CLOCK_MONOTONIC) {
        return Error(EINVAL);
    }

    this->validate_pointer_access(ts, sizeof(timespec), false);
    Duration time = TimeManager::query_time(clock_id);

    ts->tv_sec = time.seconds();
    ts->tv_nsec = time.nanoseconds();

    return 0;
}

ErrorOr<FlatPtr> Process::sys$clock_nanosleep(clockid_t clock_id, int flags, const timespec* req, timespec*) {
    if (clock_id < CLOCK_REALTIME || clock_id > CLOCK_MONOTONIC) {
        return Error(EINVAL);
    }

    bool is_absolute = flags & TIMER_ABSTIME;
    this->validate_read(req, sizeof(timespec));

    auto* thread = Thread::current();

    auto duration = Duration::from_timespec(*req);
    auto* blocker = new SleepBlocker(duration, clock_id, is_absolute);

    thread->block(blocker);
    return 0;
}

}