#pragma once

#include <sys/syscall.h>
#include <sys/types.h>

#ifdef __i386__

inline int syscall(long number) {
    int result = 0;
    asm volatile("int $0x80" : "=a"(result) : "a"(number) : "memory");

    return result;
}

inline int syscall(long number, auto arg1) {
    int result = 0;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"((uintptr_t)arg1) : "memory");

    return result;
}

inline int syscall(long number, auto arg1, auto arg2) {
    int result = 0;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"((uintptr_t)arg1), "c"((uintptr_t)arg2) : "memory");

    return result;
}

inline int syscall(long number, auto arg1, auto arg2, auto arg3) {
    int result = 0;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"((uintptr_t)arg1), "c"((uintptr_t)arg2), "d"((uintptr_t)arg3) : "memory");

    return result;
}

inline int syscall(long number, auto arg1, auto arg2, auto arg3, auto arg4) {
    int result = 0;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"((uintptr_t)arg1), "c"((uintptr_t)arg2), "d"((uintptr_t)arg3), "S"((uintptr_t)arg4) : "memory");

    return result;
}

#else

inline uintptr_t syscall(long number) {
    uintptr_t result = 0;
    asm volatile(
        "syscall" 
        : "=a"(result)
        : "a"(number)
        : "rcx", "r11", "memory"
    );

    return result;
}

inline uintptr_t syscall(long number, auto arg1) {
    uintptr_t result = 0;
    asm volatile(
        "syscall" 
        : "=a"(result)
        : "a"(number), "d"((uintptr_t)arg1)
        : "rcx", "r11", "memory"
    );

    return result;
}

inline uintptr_t syscall(long number, auto arg1, auto arg2) {
    uintptr_t result = 0;
    asm volatile(
        "syscall" 
        : "=a"(result)
        : "a"(number), "d"((uintptr_t)arg1), "D"((uintptr_t)arg2)
        : "rcx", "r11", "memory"
    );

    return result;
}

inline uintptr_t syscall(long number, auto arg1, auto arg2, auto arg3) {
    uintptr_t result = 0;
    asm volatile(
        "syscall" 
        : "=a"(result)
        : "a"(number), "d"((uintptr_t)arg1), "D"((uintptr_t)arg2), "b"((uintptr_t)arg3)
        : "rcx", "r11", "memory"
    );

    return result;
}

inline uintptr_t syscall(long number, auto arg1, auto arg2, auto arg3, auto arg4) {
    uintptr_t result = 0;
    asm volatile(
        "syscall" 
        : "=a"(result)
        : "a"(number), "d"((uintptr_t)arg1), "D"((uintptr_t)arg2), "b"((uintptr_t)arg3), "S"((uintptr_t)arg4)
        : "rcx", "r11", "memory"
    );

    return result;
}

#endif