#pragma once

#include <sys/cdefs.h>
#include <sys/types.h>
#include <stddef.h>

#ifndef _HAVE_STDIO
    #define SEEK_SET 0
    #define SEEK_CUR 1
    #define SEEK_END 2
#endif

__BEGIN_DECLS

extern char** environ;

[[gnu::noreturn]] void _exit(int status);
[[gnu::noreturn]] void _Exit(int status);

int close(int fd);
ssize_t read(int fd, void* buffer, size_t count);
ssize_t write(int fd, const void* buffer, size_t count);
off_t lseek(int fd, off_t offset, int whence);

pid_t getpid(void);
pid_t getppid(void);
pid_t gettid(void);

int dup(int old_fd);
int dup2(int old_fd, int new_fd);

pid_t fork(void);

int execv(const char* pathname, char* const argv[]);
int execve(const char* pathname, char* const argv[], char* const envp[]);
int execvp(const char* file, char* const argv[]);

char* getcwd(char* buffer, size_t size);
int chdir(const char* path);

__END_DECLS