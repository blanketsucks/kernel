#include <kernel/ctors.h>
#include <kernel/common.h>

constexpr u32 ATEXIT_MAX_FUNCS = 32;

extern "C" void* __dso_handle = nullptr;

namespace kernel {

struct atexit_func_entry_t {
    void (*func)(void*);
    void* arg;
};

atexit_func_entry_t __atexit_funcs[ATEXIT_MAX_FUNCS];
u32 __atexit_func_count = 0;

extern "C" void __cxa_atexit(void (*func)(void*), void *arg, void*) {
    if (__atexit_func_count >= ATEXIT_MAX_FUNCS) {
        return;
    }

    auto& entry = __atexit_funcs[__atexit_func_count];
    entry.func = func;
    entry.arg = arg;

    __atexit_func_count++;
}

extern "C" ctor_t _start_ctors[];
extern "C" ctor_t _end_ctors[];

extern "C" dtor_t _start_dtors[];
extern "C" dtor_t _end_dtors[];

void run_global_constructors() {
    for (ctor_t* ctor = _start_ctors; ctor < _end_ctors; ctor++) {
        (*ctor)();
    }
}

void run_global_destructors() {
    for (dtor_t* dtor = _start_dtors; dtor < _end_dtors; dtor++) {
        (*dtor)();
    }

    for (u32 i = 0; i < __atexit_func_count; i++) {
        auto& entry = __atexit_funcs[i];
        entry.func(entry.arg);
    }
}

}