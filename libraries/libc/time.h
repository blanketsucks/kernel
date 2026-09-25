#pragma once

#include <kernel/posix/time.h>
#include <sys/cdefs.h>
#include <stddef.h>

__BEGIN_DECLS

int clock_gettime(clockid_t clock_id, struct timespec* ts);
int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec* req, struct timespec* rem);

int nanosleep(const struct timespec* req, struct timespec* rem);

time_t time(time_t* tloc);

size_t strftime(char* s, size_t max, const char* format, const struct tm* tm);
size_t strftime_l(char* s, size_t max, const char* format, const struct tm* tm, locale_t locale);

clock_t clock(void);

__END_DECLS
