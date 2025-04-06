#pragma once

#include <sys/cdefs.h>
#include <sys/types.h>
#include <stddef.h>

__BEGIN_DECLS

struct dirent {
    ino_t d_ino;
    off_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[256];
};

typedef struct {
    int fd;
    struct dirent entry;
    int offset;

    char* buffer;
    size_t size;
} DIR;

DIR* opendir(const char* name);
DIR* fdopendir(int fd);
int closedir(DIR* dirp);
struct dirent* readdir(DIR* dirp);

__END_DECLS