#pragma once

#include <sys/cdefs.h>
#include <stddef.h>

__BEGIN_DECLS

#define _HAVE_STRING_H

#define PATH_MAX 255

size_t strlen(const char* s);

char* strncpy(char* dest, const char* src, size_t n);
char* strcpy(char* dest, const char* src);
char* strcat(char* dest, const char* src);
char* strncat(char* dst, const char* src, size_t ssize);
char* strchr(const char* s, int c);
char* strrchr(const char* s, int c);
int strcmp(const char* str1, const char* str2);
int strncmp(const char* str1, const char* str2, size_t n);
int strcoll(const char *s1, const char *s2);
char* strstr(const char* haystack, const char* needle);

size_t strspn(const char* s, const char* accept);
size_t strcspn(const char* s, const char* reject);

void* memset(void* dest, int c, size_t n);
void* memcpy(void* dest, const void* src, size_t n);
int memcmp(const void* s1, const void* s2, size_t n);
void* memmove(void* dest, const void* src, size_t n);

char* strerror(int err);
void perror(const char* s);

__END_DECLS