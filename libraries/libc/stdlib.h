#pragma once

#include <sys/cdefs.h>
#include <stddef.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

__BEGIN_DECLS

[[gnu::noreturn]] void exit(int status);
[[gnu::noreturn]] void abort(void);

int atexit(void (*function)(void));

int posix_openpt(int flags);

void* malloc(size_t size);
void free(void* ptr);
void* calloc(size_t nmemb, size_t size);
void* realloc(void* ptr, size_t size);

char* getenv(const char* name);

int atoi(const char* nptr);

int abs(int j);

__END_DECLS