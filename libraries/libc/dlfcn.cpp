#include <dlfcn.h>

extern "C" {

[[gnu::weak]] void* __dlopen(const char* filename, int flags);
[[gnu::weak]] void* __dlsym(void* handle, const char* symbol);
[[gnu::weak]] int __dlclose(void* handle);

void* dlopen(const char* filename, int flags) {
    if (__dlopen) {
        return __dlopen(filename, flags);
    }

    return nullptr;
}

int dlclose(void* handle) {
    if (__dlclose) {
        return __dlclose(handle);
    }

    return 0;
}

void* dlsym(void* handle, const char* symbol) {
    if (__dlsym) {
        return __dlsym(handle, symbol);
    }

    return nullptr;
}

char* dlerror() {
    return nullptr;
}


}