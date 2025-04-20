#include <std/cstring.h>

#ifndef __KERNEL__
    #include <string.h>
#endif

namespace std {

#if defined(__KERNEL__) || defined(__KERNEL_LOADER__)

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }

    return len;
}

void itoa(i32 n, char* str, u32 base) {
    if (n == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    if (n < 0 && base == 10) {
        str[0] = '-';
        n = -n;
    }

    u32 i = 0;
    while (n != 0) {
        u32 rem = n % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        n /= base;
    }

    str[i] = '\0';

    for (u32 j = 0; j < i / 2; j++) {
        char tmp = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = tmp;
    }
}

u32 strntoul(const char* str, size_t n, char** endptr, u32 base) {
    u32 result = 0;
    u32 i = 0;
    
    while (i < n) {
        u32 digit = 0;
        if (isdigit(str[i])) {
            digit = str[i] - '0';
        } else if (islower(str[i])) {
            digit = str[i] - 'a' + 10;
        } else {
            digit = str[i] - 'A' + 10;
        }

        result = result * base + digit;
        i++;
    }

    if (endptr) {
        *endptr = const_cast<char*>(str + i);
    }

    return result;
}

u32 strtoul(const char* str, char** endptr, u32 base) {
    return strntoul(str, strlen(str), endptr, base);
}

int strcmp(const char* str1, const char* str2) {
    size_t i = 0;
    while (str1[i] && str2[i] && str1[i] == str2[i]) {
        i++;
    }

    return str1[i] - str2[i];
}

int strncmp(const char* str1, const char* str2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (str1[i] != str2[i]) {
            return str1[i] - str2[i];
        }
    }

    return 0;
}

void* memset(void* dst, int c, size_t n) {
    asm volatile("rep stosb" : "+D"(dst), "+c"(n) : "a"(c) : "memory");
    return dst;
}

void* memcpy(void* dst, const void* src, size_t n) {
    asm volatile("rep movsb" : "+D"(dst), "+S"(src), "+c"(n) :: "memory");
    return dst;
}

int memcmp(const void* ptr1, const void* ptr2, size_t n) {
    const u8* p1 = static_cast<const u8*>(ptr1);
    const u8* p2 = static_cast<const u8*>(ptr2);

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }

    return 0;
}

#else

size_t strlen(const char* str) {
    return ::strlen(str);
}

void* memset(void* ptr, int c, size_t n) {
    return ::memset(ptr, c, n);
}

void* memcpy(void* dst, const void* src, size_t n) {
    return ::memcpy(dst, src, n);
}

int memcmp(const void* ptr1, const void* ptr2, size_t n) {
    return ::memcmp(ptr1, ptr2, n);
}

#endif

}