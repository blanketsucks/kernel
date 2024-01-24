#pragma once

#ifdef __KERNEL__
    #include <kernel/memory/liballoc.h>
    #include <kernel/memory/mm.h>
#else
    #include <stdlib.h>

    #define kmalloc malloc
    #define kfree free
    #define krealloc realloc
    #define kcalloc calloc

    void* operator new(size_t size) {
        return kmalloc(size);
    }

    void* operator new[](size_t size) {
        return kmalloc(size);
    }

    void operator delete(void* ptr) {
        kfree(ptr);
    }

    void operator delete[](void* ptr) {
        kfree(ptr);
    }

    void* operator new(size_t size, void* ptr) {
        return ptr;
    }

    void* operator new[](size_t size, void* ptr) {
        return ptr;
    }
#endif

