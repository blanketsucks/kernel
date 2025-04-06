#include <sched.h>

#include <sys/syscall.h>
#include <sys/syscall.hpp>

extern "C" {

int sched_yield() {
    return syscall(SYS_YIELD);
}

}