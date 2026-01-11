#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

#include <std/time.h>
#include <std/format.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        return 0;
    }

    pid_t pid = fork();
    if (!pid) {
        if (execve(argv[1], &argv[1], nullptr) < 0) {
            dbgln("Failed to exec {}: {}", argv[1], strerror(errno));
            return 1;
        }

        __builtin_unreachable();
    }

    struct timespec ts1, ts2;
    if (clock_gettime(CLOCK_REALTIME, &ts1) < 0) {
        dbgln("Failed to get time: {}", strerror(errno));
        return 1;
    }

    int status = 0;
    waitpid(pid, &status, 0);

    if (clock_gettime(CLOCK_REALTIME, &ts2) < 0) {
        dbgln("Failed to get time: {}", strerror(errno));
        return 1;
    }

    auto start = Duration::from_timespec(ts1);
    auto end = Duration::from_timespec(ts2);

    auto diff = end - start;
    dbgln("Process {} (PID {}) exited in {}.{:09} seconds", argv[1], pid, diff.seconds(), diff.nanoseconds() % 1'000'000'000);

    return 0;
}