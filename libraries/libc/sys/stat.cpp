#include <sys/stat.h>
#include <sys/syscall.hpp>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

extern "C" {

int fstat(int fd, struct stat* st) {
    return syscall(SYS_FSTAT, fd, st);
}

int stat(const char* path, struct stat* st) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    int rc = fstat(fd, st);
    close(fd);

    __set_errno_return(rc, rc, -1);
}

}