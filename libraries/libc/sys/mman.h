#pragma once

#include <kernel/posix/sys/mman.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <stddef.h>

__BEGIN_DECLS

void* mmap(void* addr, size_t size, int prot, int flags, int fd, off_t offset);
int munmap(void* addr, size_t size);

__END_DECLS