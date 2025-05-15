#include <sys/mman.h>
#include <sys/syscall.hpp>
#include <errno.h>

extern "C" {

void* mmap(void* addr, size_t size, int prot, int flags, int fd, off_t offset) {
    mmap_args args = { addr, size, prot, flags, fd, offset };
    uintptr_t ret = syscall(SYS_MMAP, reinterpret_cast<uintptr_t>(&args));

    if (ret < 0) {
        errno = -ret;
        return MAP_FAILED;
    } else {
        return reinterpret_cast<void*>(ret);
    }
}

int munmap(void*, size_t) {
    // FIXME: Implement
    return 0;
}

}