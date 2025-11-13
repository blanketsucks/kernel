#pragma once

#include <kernel/posix/sys/stat.h>
#include <sys/cdefs.h>
#include <stddef.h>

__BEGIN_DECLS

int stat_length(const char* path, size_t path_length, struct stat* buf);
int stat(const char* path, struct stat* buf);

int fstat(int fd, struct stat* buf);

__END_DECLS