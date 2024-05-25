
#include <std/types.h>

#define ATEXIT_MAX_FUNCS 1024

extern "C" {

struct atexit_func_entry_t {
    void (*func)(void*);
    void* arg;
    void* dso_handle;
};

atexit_func_entry_t __atexit_funcs[ATEXIT_MAX_FUNCS];
u32 __atexit_func_count = 0;

int __cxa_atexit(void (*func)(void*), void* arg, void* dso_handle) {
    if (__atexit_func_count >= ATEXIT_MAX_FUNCS) {
        return -1;
    }

    auto& entry = __atexit_funcs[__atexit_func_count];
    entry.func = func;
    entry.arg = arg;
    entry.dso_handle = dso_handle;

    __atexit_func_count++;
    return 0;
}

void __cxa_finalize(void* dso_handle) {
    for (u32 i = 0; i < __atexit_func_count; i++) {
        auto& entry = __atexit_funcs[i];
        if (!entry.func) {
            continue;
        }

        if (dso_handle && dso_handle != entry.dso_handle) {
            continue;
        }

        entry.func(entry.arg);
        entry.func = nullptr;
    }
}

}