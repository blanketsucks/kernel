#pragma once

#include <sys/cdefs.h>
#include <stddef.h>

__BEGIN_DECLS

#define _HAVE_STRING_H

size_t strlen(const char* s);

char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* dest, const char* src);
char* strchr(const char* s, int c);

void* memset(void* dest, int c, size_t n);
void* memcpy(void* dest, const void* src, size_t n);
int memcmp(const void* s1, const void* s2, size_t n);

__END_DECLS