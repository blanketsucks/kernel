#pragma once

#include <sys/syscall.h>

inline int syscall(long number) {
    int result = 0;
    asm volatile("int $0x80" : "=a"(result) : "a"(number) : "memory");

    return result;
}

inline int syscall(long number, auto arg1) {
    int result = 0;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"((int)arg1) : "memory");

    return result;
}

inline int syscall(long number, auto arg1, auto arg2) {
    int result = 0;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"((int)arg1), "c"((int)arg2) : "memory");

    return result;
}

inline int syscall(long number, auto arg1, auto arg2, auto arg3) {
    int result = 0;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"((int)arg1), "c"((int)arg2), "d"((int)arg3) : "memory");

    return result;
}

inline int syscall(long number, auto arg1, auto arg2, auto arg3, auto arg4) {
    int result = 0;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"((int)arg1), "c"((int)arg2), "d"((int)arg3), "S"((int)arg4) : "memory");

    return result;
}

inline int syscall(long number, auto arg1, auto arg2, auto arg3, auto arg4, auto arg5) {
    int result = 0;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"((int)arg1), "c"((int)arg2), "d"((int)arg3), "S"((int)arg4), "D"((int)arg5) : "memory");

    return result;
}