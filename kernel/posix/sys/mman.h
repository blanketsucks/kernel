#pragma once

#include <sys/types.h>
#include <stddef.h>

#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4

#define MAP_SHARED 1
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 4
#define MAP_ANON MAP_ANONYMOUS
#define MAP_FIXED 8

#define MAP_FAILED ((void*)-1)

struct mmap_args {
    void* addr;
    size_t size;
    int prot;
    int flags;
    int fd;
    off_t offset;
};
