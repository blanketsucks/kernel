#pragma once

#include <stdint.h>
#include <kernel/posix/sys/types.h>

#define TIMER_ABSTIME 1

#define LC_ALL 1
#define LC_COLLATE 2
#define LC_CTYPE 3
#define LC_MESSAGES 4
#define LC_MONETARY 5
#define LC_NUMERIC 6
#define LC_TIME 7

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

struct timeval {
    time_t       tv_sec;   /* Seconds */
    suseconds_t  tv_usec;  /* Microseconds */
};

typedef int clockid_t;
typedef void* locale_t;

enum {
    CLOCK_REALTIME,
    CLOCK_MONOTONIC,
};