#pragma once

#include <std/types.h>

namespace std {

size_t strlen(const char* str);
void itoa(i32 n, char* str, u32 base);
void atoi(const char* str, u32* out, u32 base);

extern "C" {

void* memset(void* ptr, int c, size_t n);
void* memcpy(void* dst, const void* src, size_t n);
int memcmp(const void* ptr1, const void* ptr2, size_t n);

}

}