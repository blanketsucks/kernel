#include <sys/stat.h>
#include <sys/syscall.hpp>

extern "C" {

int fstat(int fd, struct stat* st) {
    return syscall(SYS_FSTAT, fd, st);
}

}