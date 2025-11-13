#pragma once

#include <sys/cdefs.h>
#include <sys/types.h>
#include <stddef.h>
#include <kernel/posix/fcntl.h>

__BEGIN_DECLS

int open_length(const char* pathname, size_t pathname_length, int flags, mode_t mode);
int open(const char* pathname, int flags, ...);

__END_DECLS