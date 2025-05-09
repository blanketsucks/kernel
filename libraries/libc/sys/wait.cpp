#include <sys/wait.h>
#include <sys/syscall.hpp>
#include <errno.h>

extern "C" {

int waitpid(pid_t pid, int* status, int options) {
    int ret = syscall(SYS_WAITPID, pid, status, options);
    __set_errno_return(ret, ret, -1);
}

}