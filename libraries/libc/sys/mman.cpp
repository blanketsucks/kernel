#include <sys/mman.h>
#include <sys/syscall.hpp>

extern "C" {

void* mmap(void* addr, size_t size, int prot, int flags, int fd, off_t offset) {
    mmap_args args = { addr, size, prot, flags, fd, offset };
    return reinterpret_cast<void*>(syscall(SYS_MMAP, reinterpret_cast<uintptr_t>(&args)));
}

int munmap(void* addr, size_t size) {
    // FIXME: Implement
    return 0;
}

}