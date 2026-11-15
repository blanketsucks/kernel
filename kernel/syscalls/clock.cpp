#include <kernel/process/process.h>
#include <kernel/process/threads.h>
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

    auto duration = Duration::from_timespec(*req);

    Deadline deadline;
    if (is_absolute) {
        deadline = Deadline::at(duration, clock_id);
    } else {
        deadline = Deadline::after(duration, clock_id);
    }

    WaitQueue wq;
    wq.prepare_and_wait([]() {}, deadline);

    return 0;
}

}
