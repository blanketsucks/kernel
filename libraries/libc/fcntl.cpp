#include <fcntl.h>
#include <stdarg.h>

#include <sys/syscall.hpp>
#include <sys/types.h>

extern "C" {

int open(const char* pathname, int flags, ...) {
    va_list ap;
    va_start(ap, flags);

    mode_t mode = va_arg(ap, int);
    va_end(ap);

    return syscall(SYS_OPEN, pathname, flags, mode);
}

}