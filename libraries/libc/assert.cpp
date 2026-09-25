#include <assert.h>

#include <stdlib.h>

extern "C" {

// TODO: Properly implement
[[gnu::noreturn]] void __assert_failed(const char* file, int line, const char* func, const char* expr) {
    abort();
}
    
}