#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <std/format.h>

int main() {
    pid_t pid = fork();
    if (!pid) {
        execve("/bin/ls", nullptr, nullptr);
    }

    int status = 0;
    waitpid(pid, &status, 0);

    return 42;
}