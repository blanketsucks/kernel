#pragma once

#define __SYSCALL_LIST(Op)      \
    Op(exit)                    \
    Op(open)                    \
    Op(close)                   \
    Op(read)                    \
    Op(write)                   \
    Op(lseek)                   \
    Op(readdir)                 \
    Op(stat)                    \
    Op(fstat)                   \
    Op(mmap)                    \
    Op(munmap)                  \
    Op(getpid)                  \
    Op(getppid)                 \
    Op(gettid)                  \
    Op(dup)                     \
    Op(dup2)                    \
    Op(getcwd)                  \
    Op(chdir)                   \
    Op(ioctl)                   \
    Op(fork)                    \
    Op(execve)                  \
    Op(waitpid)                 \
    Op(clock_gettime)           \
    Op(clock_nanosleep)

enum {
#define Op(name) SYS_##name,
    __SYSCALL_LIST(Op)
#undef Op
};