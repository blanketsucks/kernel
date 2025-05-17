#pragma once

#include <kernel/posix/time.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

int clock_gettime(clockid_t clock_id, struct timespec* ts);
int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec* req, struct timespec* rem);

int nanosleep(const struct timespec* req, struct timespec* rem);
int usleep(useconds_t usec);
unsigned int sleep(unsigned int seconds);

__END_DECLS
