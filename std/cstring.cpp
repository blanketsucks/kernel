#include <std/cstring.h>

namespace std {

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

void atoi(const char* str, u32* out, u32 base) {
    u32 result = 0;
    u32 i = 0;
    
    while (str[i]) {
        u32 digit = str[i] - '0';
        result = result * base + digit;

        i++;
    }

    *out = result;
}

extern "C" {

void* memset(void* ptr, int c, size_t n) {
    u8* bytes = reinterpret_cast<u8*>(ptr);
    for (size_t i = 0; i < n; i++) {
        bytes[i] = c;
    }

    return ptr;
}

void* memcpy(void* dst, const void* src, size_t n) {
    u8* dst_bytes = reinterpret_cast<u8*>(dst);
    const u8* src_bytes = reinterpret_cast<const u8*>(src);
    for (size_t i = 0; i < n; i++) {
        dst_bytes[i] = src_bytes[i];
    }

    return dst;
}

int memcmp(const void* ptr1, const void* ptr2, size_t n) {
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

}