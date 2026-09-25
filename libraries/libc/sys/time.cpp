#include <sys/time.h>
#include <time.h>

extern "C" {

int gettimeofday(struct timeval* tv, struct timezone*) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) < 0) {
        return -1;
    }

    tv->tv_sec  = ts.tv_sec;
    tv->tv_usec = ts.tv_nsec * 1000;

    return 0;
}

}