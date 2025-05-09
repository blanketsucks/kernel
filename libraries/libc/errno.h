#pragma once

#include <kernel/posix/errno.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

#define __set_errno_return(ret, success, err)   \
    if (ret < 0) {                              \
        errno = -ret;                           \
        return err;                             \
    } else {                                    \
        return success;                         \
    }


int* __errno_location() __attribute__((const));
#define errno (*__errno_location())

__END_DECLS