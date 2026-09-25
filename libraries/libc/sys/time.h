#pragma once

#include <sys/cdefs.h>
#include <kernel/posix/time.h>

__BEGIN_DECLS

int gettimeofday(struct timeval* tv, struct timezone* tz);

__END_DECLS