#include <signal.h>
#include <errno.h>

extern "C" {

sighandler_t signal(int signum, sighandler_t handler) {
    errno = ENOSYS;
    return nullptr;
}

}