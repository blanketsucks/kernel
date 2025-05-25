#pragma once

#ifdef __KERNEL__
    #include <kernel/memory/liballoc.h>
    #include <kernel/memory/stdlib.h>
#else
    #include <stdlib.h>

    #define kmalloc malloc
    #define kfree free
    #define krealloc realloc
    #define kcalloc calloc

    inline void* operator new(size_t, void* p) { return p; }
    inline void* operator new[](size_t, void* p) { return p; }

    inline void* operator new(size_t size) {
        return malloc(size);
    }

    inline void* operator new[](size_t size) {
        return malloc(size);
    }

    inline void operator delete(void* p) {
        free(p);
    }

    inline void operator delete[](void* p) {
        free(p);
    }

    inline void operator delete(void* p, size_t) {
        free(p);
    }
#endif

