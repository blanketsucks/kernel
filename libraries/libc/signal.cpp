#include <signal.h>
#include <errno.h>
#include <stdlib.h>

extern "C" {

sighandler_t signal(int signum, sighandler_t handler) {
    errno = ENOSYS;
    return nullptr;
}

int kill(pid_t pid, int sig) {
    abort();
}

}