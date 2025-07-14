#include <fcntl.h>
#include <sched.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

#include <std/format.h>

int main() {
    char* ptr = (char*)mmap(NULL, 4096 * 2, PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    dbgln("ptr={}", (void*)ptr);

    void(*func)() = (void(*)())ptr;
    func();

    // bool* ptr = (bool*)mmap(nullptr, sizeof(bool), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    // dbgln("Forking process.");
    // pid_t pid = fork();
    // if (pid == 0) {
    //     dbgln("Child writing to memory...");
    //     *ptr = true;
    // } else {
    //     dbgln("Parent waiting for child to write to memory...");
    //     while (*ptr == false) {}

    //     dbgln("Child wrote to memory, exiting...");
    // }

    return 32;
}