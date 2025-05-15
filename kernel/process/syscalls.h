#pragma once

#define __SYSCALL_LIST(Op)      \
    Op(SYS_EXIT, exit)          \
    Op(SYS_OPEN, open)          \
    Op(SYS_CLOSE, close)        \
    Op(SYS_READ, read)          \
    Op(SYS_WRITE, write)        \
    Op(SYS_LSEEK, lseek)        \
    Op(SYS_READDIR, readdir)    \
    Op(SYS_FSTAT, fstat)        \
    Op(SYS_MMAP, mmap)          \
    Op(SYS_GETPID, getpid)      \
    Op(SYS_GETPPID, getppid)    \
    Op(SYS_GETTID, gettid)      \
    Op(SYS_DUP, dup)            \
    Op(SYS_DUP2, dup2)          \
    Op(SYS_GETCWD, getcwd)      \
    Op(SYS_CHDIR, chdir)        \
    Op(SYS_IOCTL, ioctl)        \
    Op(SYS_FORK, fork)          \
    Op(SYS_EXECVE, execve)      \
    Op(SYS_WAITPID, waitpid)


enum {
#define Op(name, func) name,
    __SYSCALL_LIST(Op)
#undef Op
};