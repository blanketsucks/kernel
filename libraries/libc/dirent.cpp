#include <dirent.h>
#include <sys/cdefs.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.hpp>
#include <errno.h>
#include <std/format.h>

#include <std/types.h>

__BEGIN_DECLS

struct [[gnu::packed]] sys_dirent {
    ino_t inode;
    u8 type;
    size_t name_length;
    char name[];
};

DIR* opendir(const char* name) {
    int fd = open(name, O_RDONLY | O_DIRECTORY);
    return fdopendir(fd);
}

DIR* fdopendir(int fd) {
    if (fd < 0) {
        return nullptr;
    }

    auto* dirp = new DIR;

    dirp->fd = fd;
    dirp->offset = 0;
    dirp->buffer = nullptr;
    dirp->size = 0;

    return dirp;
}

int closedir(DIR* dirp) {
    if (!dirp || dirp->fd < 0) {
        return -1;
    }

    if (dirp->buffer) {
        free(dirp->buffer);
    }

    close(dirp->fd);
    delete dirp;

    return 0;
}

struct dirent* readdir(DIR* dirp) {
    if (!dirp || dirp->fd < 0) {
        return nullptr;
    }

    if (!dirp->buffer) {
        size_t allocation_size = 4096;
        dirp->buffer = reinterpret_cast<char*>(malloc(allocation_size));

        if (!dirp->buffer) {
            return nullptr;
        }

        while (true) {
            ssize_t nread = syscall(SYS_readdir, dirp->fd, dirp->buffer, allocation_size);
            if (nread < 0) {
                if (nread == -EINVAL) {
                    allocation_size *= 2;
                    dirp->buffer = reinterpret_cast<char*>(realloc(dirp->buffer, allocation_size));

                    if (!dirp->buffer) {
                        return nullptr;
                    }

                    continue;
                }

                free(dirp->buffer);
                dirp->buffer = nullptr;

                return nullptr;
            }

            dirp->size = nread;
            break;
        }    
    }

    if (dirp->offset + sizeof(sys_dirent) > dirp->size) {
        return nullptr;
    }

    auto* sys = reinterpret_cast<sys_dirent*>(dirp->buffer + dirp->offset);
    dirp->offset += sizeof(sys_dirent) + sys->name_length;

    auto* dirent = &dirp->entry;

    dirent->d_ino = sys->inode;
    dirent->d_type = sys->type;
    dirent->d_reclen = sizeof(sys_dirent) + sys->name_length;
    dirent->d_off = dirp->offset;

    for (size_t i = 0; i < sys->name_length; i++) {
        dirent->d_name[i] = sys->name[i];
    }

    dirent->d_name[sys->name_length] = '\0';
    return dirent;
}

__END_DECLS