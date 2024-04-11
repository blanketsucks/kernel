#include <std/cstring.h>

namespace std {

constexpr bool isdigit(char c) {
    return c >= '0' && c <= '9';
}

constexpr bool isalpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

constexpr bool islower(char c) {
    return c >= 'a' && c <= 'z';
}

constexpr bool tolower(char c) {
    return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}

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

extern "C" void* memset(void* ptr, int c, size_t n) {
    u8* bytes = reinterpret_cast<u8*>(ptr);
    for (size_t i = 0; i < n; i++) {
        bytes[i] = c;
    }

    return ptr;
}

extern "C" void* memcpy(void* dst, const void* src, size_t n) {
    u8* dst_bytes = reinterpret_cast<u8*>(dst);
    const u8* src_bytes = reinterpret_cast<const u8*>(src);
    for (size_t i = 0; i < n; i++) {
        dst_bytes[i] = src_bytes[i];
    }

    return dst;
}

extern "C" int memcmp(const void* ptr1, const void* ptr2, size_t n) {
    const u8* bytes1 = reinterpret_cast<const u8*>(ptr1);
    const u8* bytes2 = reinterpret_cast<const u8*>(ptr2);

    for (size_t i = 0; i < n; i++) {
        if (bytes1[i] != bytes2[i]) {
            return bytes1[i] - bytes2[i];
        }
    }

    return 0;
}

}