#pragma once

#include <stdint.h>
#include <kernel/posix/sys/types.h>

#define TIMER_ABSTIME 1

struct timespec {
    time_t tv_sec;
    long tv_nsec;
};

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

typedef int clockid_t;

enum {
    CLOCK_REALTIME,
    CLOCK_MONOTONIC,
};