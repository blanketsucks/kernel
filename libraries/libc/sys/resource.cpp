#include <sys/resource.h>

extern "C" {

int getrlimit(int, struct rlimit* rlim) {
    rlim->rlim_cur = RLIM_INFINITY;
    rlim->rlim_max = RLIM_INFINITY;

    return 0;
}

}