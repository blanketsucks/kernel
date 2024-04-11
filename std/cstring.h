#pragma once

#include <std/types.h>

namespace std {

size_t strlen(const char* str);
void itoa(i32 n, char* str, u32 base);

u32 strtoul(const char* str, char** endptr, u32 base);
u32 strntoul(const char* str, size_t n, char** endptr, u32 base);

int strcmp(const char* str1, const char* str2);
int strncmp(const char* str1, const char* str2, size_t n);

extern "C" {

void* memset(void* ptr, int c, size_t n);
void* memcpy(void* dst, const void* src, size_t n);
int memcmp(const void* ptr1, const void* ptr2, size_t n);

}

}