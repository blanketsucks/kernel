#include <string.h>
#include <std/types.h>

extern "C" {

size_t strlen(const char* s) {
    size_t len = 0;
    while (*(s++)) {
        ++len;
    }

    return len;
}

void* memset(void* dest, int c, size_t n) {
    asm volatile("rep stosb" : "+D"(dest), "+c"(n) : "a"(c) : "memory");
    return dest;
}

void* memcpy(void* dest, const void* src, size_t n) {
    asm volatile("rep movsb" : "+D"(dest), "+S"(src), "+c"(n) : : "memory");
    return dest;
}

int memcmp(const void* ptr1, const void* ptr2, size_t n) {
    auto* s1 = static_cast<const u8*>(ptr1);
    auto* s2 = static_cast<const u8*>(ptr2);

    while (n-- > 0) {
        if (*s1++ != *s2++) {
            return s1[-1] < s2[-1] ? -1 : 1;
        }
    }
    return 0;
}

}