#include <sys/ioctl.h>
#include <sys/syscall.hpp>
#include <stdarg.h>

extern "C" {

int ioctl(int fd, unsigned request, ...) {
    va_list ap;
    va_start(ap, request);
    unsigned arg = va_arg(ap, unsigned);
    va_end(ap);
    return syscall(SYS_IOCTL, fd, request, arg);
}

}