#include <dlfcn.h>

extern "C" {

void* dlopen(const char*, int) {
    return nullptr;
}

int dlclose(void*) {
    return 0;
}

void* dlsym(void*, const char*) {
    return nullptr;
}

char* dlerror() {
    return nullptr;
}


}