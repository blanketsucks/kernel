#pragma once

#include <std/types.h>

#ifdef __KERNEL__

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

size_t strlen(const char* str);
void itoa(i32 n, char* str, u32 base);

u32 strtoul(const char* str, char** endptr, u32 base);
u32 strntoul(const char* str, size_t n, char** endptr, u32 base);

u64 strtoull(const char* str, char** endptr, u32 base);
u64 strntoull(const char* str, size_t n, char** endptr, u32 base);

int strcmp(const char* str1, const char* str2);
int strncmp(const char* str1, const char* str2, size_t n);


extern "C" {

void* memset(void* ptr, int c, size_t n);
void* memcpy(void* dst, const void* src, size_t n);
int memcmp(const void* ptr1, const void* ptr2, size_t n);

}

}

using std::memset;
using std::memcpy;
using std::memcmp;

#else

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

size_t strlen(const char* str);
void itoa(i32 n, char* str, u32 base);

u32 strtoul(const char* str, char** endptr, u32 base);
u32 strntoul(const char* str, size_t n, char** endptr, u32 base);

int strcmp(const char* str1, const char* str2);

void* memset(void* ptr, int c, size_t n);
void* memcpy(void* dst, const void* src, size_t n);
int memcmp(const void* ptr1, const void* ptr2, size_t n);

}

#endif
