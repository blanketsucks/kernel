#include <sys/syscall.hpp>
#include <unistd.h>
#include <fcntl.h>
#include <std/format.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/mman.h>

void yield() {
    syscall(SYS_YIELD);
}

int main() {
    bool* ptr = (bool*)mmap(nullptr, sizeof(bool), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    dbgln("Forking process.");
    pid_t pid = fork();
    if (pid == 0) {
        dbgln("Child writing to memory...");
        *ptr = true;

        yield();
    } else {
        dbgln("Parent waiting for child to write to memory...");
        while (*ptr == false) {
            yield();
        }

        dbgln("Child wrote to memory, exiting...");
    }

    return pid;
}