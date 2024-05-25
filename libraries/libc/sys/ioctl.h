#pragma once

#include <kernel/posix/sys/ioctl.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

int ioctl(int fd, unsigned request, ...);


__END_DECLS