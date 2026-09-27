#pragma once

#include <sys/cdefs.h>
#include <sys/types.h>
#include <kernel/posix/unistd.h>
#include <stddef.h>

#ifndef _HAVE_STDIO
    #define SEEK_SET 0
    #define SEEK_CUR 1
    #define SEEK_END 2
#endif

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

__BEGIN_DECLS

extern char** environ;

[[gnu::noreturn]] void _exit(int status);
[[gnu::noreturn]] void _Exit(int status);

int usleep(useconds_t usec);
unsigned int sleep(unsigned int seconds);

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

int execlp(const char* file, const char* arg, ...);
int execv(const char* pathname, char* const argv[]);
int execve(const char* pathname, char* const argv[], char* const envp[]);
int execvp(const char* file, char* const argv[]);

char* getcwd(char* buffer, size_t size);
int chdir(const char* path);

int access(const char* path, mode_t mode);

int isatty(int fd);

int unlink(const char* pathname);

char* ttyname(int fd);
int ttyname_r(int fd, char* buf, size_t buflen);

int pipe(int pipefd[2]);

long fpathconf(int fd, int name);
long pathconf(const char* path, int name);

uid_t getuid(void);
uid_t geteuid(void);

__END_DECLS