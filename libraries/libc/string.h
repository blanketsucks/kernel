#pragma once

#include <sys/cdefs.h>
#include <stddef.h>

__BEGIN_DECLS

#define _HAVE_STRING_H

size_t strlen(const char* s);

void* memset(void* dest, int c, size_t n);
void* memcpy(void* dest, const void* src, size_t n);
int memcmp(const void* s1, const void* s2, size_t n);

__END_DECLS