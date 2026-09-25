#include <time.h>
#include <errno.h>
#include <sys/syscall.hpp>

extern "C" {

int clock_gettime(clockid_t clock_id, struct timespec* ts) {
    int ret = syscall(SYS_clock_gettime, clock_id, ts);
    __set_errno_return(ret, 0, -1);
}

int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec* req, struct timespec* rem) {
    int ret = syscall(SYS_clock_nanosleep, clock_id, flags, req, rem);
    __set_errno_return(ret, 0, -1);
}

int nanosleep(const struct timespec* req, struct timespec* rem) {
    return clock_nanosleep(CLOCK_REALTIME, 0, req, rem);
}

}