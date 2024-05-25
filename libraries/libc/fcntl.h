#pragma once

#include <sys/cdefs.h>
#include <kernel/posix/fcntl.h>

__BEGIN_DECLS

int open(const char* pathname, int flags, ...);

__END_DECLS