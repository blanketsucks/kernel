#include <sys/stat.h>
#include <sys/syscall.hpp>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

extern "C" {

int fstat(int fd, struct stat* st) {
    return syscall(SYS_fstat, fd, st);
}

int stat_length(const char* path, size_t path_length, struct stat* st) {
    int ret = syscall(SYS_stat, path, path_length, st);
    __set_errno_return(ret, ret, -1);
}

int stat(const char* path, struct stat* st) {
    return stat_length(path, strlen(path), st);
}

}