#pragma once

#include <sys/cdefs.h>

__BEGIN_DECLS

#define RTLD_DEFAULT 0
#define RTLD_LAZY 2
#define RTLD_NOW 4
#define RTLD_LOCAL 8
#define RTLD_GLOBAL 16

void* dlopen(const char* filename, int flags);
int dlclose(void* handle);

void* dlsym(void* handle, const char* symbol);
char* dlerror(void);

__END_DECLS