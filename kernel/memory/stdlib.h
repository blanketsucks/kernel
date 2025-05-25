#pragma once

#include <kernel/common.h>

void* operator new(size_t size);
inline void* operator new(size_t, void* p) { return p; }

void* operator new[](size_t size);
inline void* operator new[](size_t, void* p) { return p; }

void operator delete(void* p);
void operator delete[](void* p);

void operator delete(void* p, size_t size);
void operator delete[](void* p, size_t size);