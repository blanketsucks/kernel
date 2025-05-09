#include <unistd.h>
#include <string.h>
#include <sys/syscall.hpp>
#include <errno.h>

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
    int ret = syscall(SYS_CLOSE, fd);
    __set_errno_return(ret, 0, -1);
}

ssize_t read(int fd, void* buffer, size_t count) {
    ssize_t n = syscall(SYS_READ, fd, buffer, count);
    __set_errno_return(n, n, -1);
}

ssize_t write(int fd, const void* buffer, size_t count) {
    ssize_t n = syscall(SYS_WRITE, fd, buffer, count);
    __set_errno_return(n, n, -1);
}

off_t lseek(int fd, off_t offset, int whence) {
    off_t ret = syscall(SYS_LSEEK, fd, offset, whence);
    __set_errno_return(ret, ret, -1);
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
    int ret = syscall(SYS_DUP, old_fd);
    __set_errno_return(ret, ret, -1);
}

int dup2(int old_fd, int new_fd) {
    int ret = syscall(SYS_DUP2, old_fd, new_fd);
    __set_errno_return(ret, ret, -1);
}

char* getcwd(char* buffer, size_t size) {
    int ret = syscall(SYS_GETCWD, buffer, size);
    __set_errno_return(ret, buffer, NULL);
}

int chdir(const char* path) {
    int ret = syscall(SYS_CHDIR, path);
    __set_errno_return(ret, 0, -1);
}

pid_t fork(void) {
    int ret = syscall(SYS_FORK);
    __set_errno_return(ret, ret, -1);
}

int execve(const char* path, char* const argv[], char* const envp[]) {
    int ret = syscall(SYS_EXECVE, path, argv, envp);
    __set_errno_return(ret, 0, -1);
}

}