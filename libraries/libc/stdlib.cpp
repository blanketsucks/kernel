#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

#include <std/format.h>

extern "C" {

extern int __cxa_atexit(void (*func)(void*), void* arg, void* dso_handle);
extern void __cxa_finalize(void* dso_handle);
extern void _fini();

[[gnu::weak]] void __call_fini_functions() { _fini(); }

char** environ;

[[gnu::noreturn]] void exit(int status) {
    // TODO: Do extra cleanup tasks once we can actually do them.
    __cxa_finalize(nullptr);
    __call_fini_functions();

    _exit(status);
}

void abort(void) {
    // TODO: Send SIGABRT signal to the current process to terminate it.
    _exit(EXIT_FAILURE);
}

static void __atexit_wrapper(void* function) {
    reinterpret_cast<void(*)(void)>(function)();
}

int atexit(void (*function)(void)) {
    return __cxa_atexit(__atexit_wrapper, reinterpret_cast<void*>(function), nullptr);
}

int posix_openpt(int flags) {
    return open("/dev/ptmx", flags);
}

static char s_ptsname_buffer[64];
char* ptsname(int fd) {
    if (ptsname_r(fd, s_ptsname_buffer, sizeof(s_ptsname_buffer)) < 0) {
        return nullptr;
    }

    return s_ptsname_buffer;
}

int ptsname_r(int fd, char* buffer, size_t size) {
    int pts = -1;
    if (ioctl(fd, TIOCGPTN, &pts) < 0) {
        return -1;
    }

    std::FormatBuffer devfs;
    devfs.append("/dev/pts/");

    if (pts < 0) {
        errno = EINVAL;
        return -1;
    }

    devfs.appendf("%d", pts);
    if (devfs.position() > size) {
        errno = ERANGE;
        return -1;
    }

    StringView path = devfs.view();

    memset(buffer, 0, path.size() + 1);
    memcpy(buffer, path.data(), path.size());

    return 0;
}

    
}