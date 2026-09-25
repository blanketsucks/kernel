#pragma once

#include <sys/cdefs.h>
#include <stddef.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

__BEGIN_DECLS

extern char *optarg;
extern int optind, opterr, optopt;

[[gnu::noreturn]] void exit(int status);
[[gnu::noreturn]] void abort(void);

int atexit(void (*function)(void));

int posix_openpt(int flags);
char* ptsname(int fd);
int ptsname_r(int fd, char* buf, size_t buflen);

void* malloc(size_t size);
void free(void* ptr);
void* calloc(size_t nmemb, size_t size);
void* realloc(void* ptr, size_t size);

char* getenv(const char* name);
int setenv(const char* name, const char* value, int overwrite);
int unsetenv(const char* name);
int putenv(char* string);

int atoi(const char* nptr);
long strtol(const char* nptr, char** endptr, int base);
long long strtoll(const char* nptr, char** endptr, int base);

int abs(int j);

void qsort(void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*));
void qsort_r(void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*, void*), void* arg);

int wctomb(char* s, wchar_t wc);

int getopt(int argc, char *argv[], const char *optstring);



__END_DECLS