#include <unistd.h>
#include <string.h>
#include <sys/syscall.hpp>

extern "C" {

[[gnu::noreturn]] void _exit(int status) {
    syscall(SYS_EXIT, status);
    __builtin_trap();
}

[[gnu::noreturn]] void _Exit(int status) {
    _exit(status);
}

// FIXME: Implement `errno` properly.

int close(int fd) {
    return syscall(SYS_CLOSE, fd);
}

ssize_t read(int fd, void* buffer, size_t count) {
    return syscall(SYS_READ, fd, buffer, count);
}

ssize_t write(int fd, const void* buffer, size_t count) {
    return syscall(SYS_WRITE, fd, buffer, count);
}

off_t lseek(int fd, off_t offset, int whence) {
    return syscall(SYS_LSEEK, fd, offset, whence);
}

pid_t getpid(void) {
    return syscall(SYS_GETPID);
}

pid_t getppid(void) {
    return syscall(SYS_GETPPID);
}

pid_t gettid(void) {
    return syscall(SYS_GETTID);
}

int dup(int old_fd) {
    return syscall(SYS_DUP, old_fd);
}

int dup2(int old_fd, int new_fd) {
    return syscall(SYS_DUP2, old_fd, new_fd);
}

char* getcwd(char* buffer, size_t size) {
    syscall(SYS_GETCWD, buffer, size);
    return buffer;
}

int chdir(const char* path) {
    return syscall(SYS_CHDIR, path);
}

pid_t fork(void) {
    return syscall(SYS_FORK);
}

int execve(const char* path, char* const argv[], char* const envp[]) {
    return syscall(SYS_EXECVE, path, argv, envp);
}

}