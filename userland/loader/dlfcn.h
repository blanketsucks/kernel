#pragma once

#include <loader/object.h>

extern "C" {

void* __dlopen(const char* filename, int flags);
void* __dlsym(void* handle, const char* symbol);
int __dlclose(void* handle);

}