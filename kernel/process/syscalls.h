#pragma once

enum {
    SYS_EXIT,

    SYS_OPEN,
    SYS_CLOSE,
    SYS_READ,
    SYS_WRITE,
    SYS_LSEEK,

    SYS_READDIR,

    SYS_FSTAT,

    SYS_MMAP,

    SYS_GETPID,
    SYS_GETPPID,
    SYS_GETTID,

    SYS_DUP,
    SYS_DUP2,

    SYS_GETCWD,
    SYS_CHDIR,

    SYS_IOCTL,

    SYS_FORK,
    SYS_EXECVE,

    SYS_YIELD,
};