#pragma once

#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

struct utimbuf {
    time_t actime;
    time_t modtime;
};

int utime(const char* filename, const struct utimbuf* times);

__END_DECLS