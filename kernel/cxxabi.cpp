#include <kernel/common.h>

__extension__ typedef int __guard __attribute__((mode(__DI__)));

extern "C" {

int __cxa_guard_acquire(__guard* g) {
    return !*(char*)(g);
}

void __cxa_guard_release(__guard* g) {
    *(char*)g = 1;
}

void __cxa_guard_abort(__guard*) {}

void __cxa_pure_virtual() {}

}