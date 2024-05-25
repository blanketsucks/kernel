#pragma once

#include <kernel/posix/sys/stat.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

int fstat(int fd, struct stat* buf);

__END_DECLS