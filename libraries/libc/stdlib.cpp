#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

extern "C" {

extern int __cxa_atexit(void (*func)(void*), void* arg, void* dso_handle);
extern void __cxa_finalize(void* dso_handle);
extern void _fini();

char** environ;

[[gnu::noreturn]] void exit(int status) {
    // TODO: Do extra cleanup tasks once we can actually do them.
    __cxa_finalize(nullptr);
    _fini();

    _exit(status);
}

void abort(void) {
    // TODO: Send SIGABRT signal to the current process to terminate it.
    _exit(EXIT_FAILURE);
}

static void __atexit_wrapper(void* function) {
    reinterpret_cast<void (*)(void)>(function)();
}

int atexit(void (*function)(void)) {
    return __cxa_atexit(__atexit_wrapper, reinterpret_cast<void*>(function), nullptr);
}

int posix_openpt(int flags) {
    return open("/dev/ptmx", flags);
}
    
}