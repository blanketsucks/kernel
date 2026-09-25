#pragma once

#include <sys/cdefs.h>
#include <kernel/posix/sys/resource.h>

__BEGIN_DECLS

struct rlimit {
    rlim_t rlim_cur;
    rlim_t rlim_max;
};

int getrlimit(int option, struct rlimit* rlim);


__END_DECLS