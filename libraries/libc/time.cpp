#include <time.h>
#include <errno.h>
#include <sys/syscall.hpp>

extern "C" {

int clock_gettime(clockid_t clock_id, struct timespec* ts) {
    int ret = syscall(SYS_CLOCK_GETTIME, clock_id, ts);
    __set_errno_return(ret, 0, -1);
}

int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec* req, struct timespec* rem) {
    int ret = syscall(SYS_CLOCK_NANOSLEEP, clock_id, flags, req, rem);
    __set_errno_return(ret, 0, -1);
}

int nanosleep(const struct timespec* req, struct timespec* rem) {
    return clock_nanosleep(CLOCK_REALTIME, 0, req, rem);
}

int usleep(useconds_t usec) {
    struct timespec req;
    req.tv_sec = usec / 1'000'000;
    req.tv_nsec = (usec % 1'000'000) * 1'000;

    return nanosleep(&req, nullptr);
}

unsigned int sleep(unsigned int seconds) {
    struct timespec req;
    req.tv_sec = seconds;
    req.tv_nsec = 0;

    struct timespec rem;
    nanosleep(&req, &rem); // FIXME: Should this set errno?

    return rem.tv_sec;
}

}