#pragma once

namespace kernel {

extern "C" void __cxa_atexit(void (*func)(void*), void *arg, void* dso_handle);

using ctor_t = void(*)();
using dtor_t = void(*)();

void run_global_constructors();
void run_global_destructors();

}