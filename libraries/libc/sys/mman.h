#pragma once

#include <kernel/posix/sys/mman.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <stddef.h>

__BEGIN_DECLS

void* mmap(void* addr, size_t size, int prot, int flags, int fd, off_t offset);
int munmap(void* addr, size_t size);

int mmap_set_name(void* addr, const char* name);
int mmap_set_name_length(void* addr, const char* name, size_t length);

__END_DECLS