#pragma once

#include <kernel/posix/sys/stat.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

int stat(const char* path, struct stat* buf);
int fstat(int fd, struct stat* buf);

__END_DECLS