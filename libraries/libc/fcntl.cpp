#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/syscall.hpp>
#include <sys/types.h>
#include <string.h>

extern "C" {

int open_length(const char* pathname, size_t pathname_length, int flags, mode_t mode) {
    int ret = syscall(SYS_open, pathname, pathname_length, flags, mode);
    __set_errno_return(ret, ret, -1);
}

int open(const char* pathname, int flags, ...) {
    va_list ap;
    va_start(ap, flags);

    mode_t mode = va_arg(ap, int);
    va_end(ap);

    return open_length(pathname, strlen(pathname), flags, mode);
}

}