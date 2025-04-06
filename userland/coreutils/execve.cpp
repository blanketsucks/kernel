#include <unistd.h>

#include <std/format.h>

int main() {
    if (fork()) {
        dbgln("Parent process");
    } else {
        dbgln("Child process");
        execve("/bin/shell", nullptr, nullptr);
    }

    return 42;
}