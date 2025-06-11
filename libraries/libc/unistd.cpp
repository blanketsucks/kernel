#include <unistd.h>
#include <string.h>
#include <sys/syscall.hpp>
#include <errno.h>

extern "C" {

[[gnu::noreturn]] void _exit(int status) {
    syscall(SYS_exit, status);
    __builtin_trap();
}

[[gnu::noreturn]] void _Exit(int status) {
    _exit(status);
}

// FIXME: Implement `errno` properly.

int close(int fd) {
    int ret = syscall(SYS_close, fd);
    __set_errno_return(ret, 0, -1);
}

ssize_t read(int fd, void* buffer, size_t count) {
    ssize_t n = syscall(SYS_read, fd, buffer, count);
    __set_errno_return(n, n, -1);
}

ssize_t write(int fd, const void* buffer, size_t count) {
    ssize_t n = syscall(SYS_write, fd, buffer, count);
    __set_errno_return(n, n, -1);
}

off_t lseek(int fd, off_t offset, int whence) {
    off_t ret = syscall(SYS_lseek, fd, offset, whence);
    __set_errno_return(ret, ret, -1);
}

pid_t getpid(void) {
    return syscall(SYS_getpid);
}

pid_t getppid(void) {
    return syscall(SYS_getppid);
}

pid_t gettid(void) {
    return syscall(SYS_gettid);
}

int dup(int old_fd) {
    int ret = syscall(SYS_dup, old_fd);
    __set_errno_return(ret, ret, -1);
}

int dup2(int old_fd, int new_fd) {
    int ret = syscall(SYS_dup2, old_fd, new_fd);
    __set_errno_return(ret, ret, -1);
}

char* getcwd(char* buffer, size_t size) {
    int ret = syscall(SYS_getcwd, buffer, size);
    __set_errno_return(ret, buffer, NULL);
}

int chdir(const char* path) {
    int ret = syscall(SYS_chdir, path);
    __set_errno_return(ret, 0, -1);
}

pid_t fork(void) {
    int ret = syscall(SYS_fork);
    __set_errno_return(ret, ret, -1);
}

int execve(const char* path, char* const argv[], char* const envp[]) {
    int ret = syscall(SYS_execve, path, argv, envp);
    __set_errno_return(ret, 0, -1);
}

}