#include <sys/mman.h>
#include <sys/syscall.hpp>
#include <errno.h>

extern "C" {

void* mmap(void* addr, size_t size, int prot, int flags, int fd, off_t offset) {
    mmap_args args = { addr, size, prot, flags, fd, offset };
    uintptr_t ret = syscall(SYS_mmap, reinterpret_cast<uintptr_t>(&args));

    if (ret > static_cast<uintptr_t>(-EMAXERRNO)) {
        errno = -ret;
        return MAP_FAILED;
    } else {
        return reinterpret_cast<void*>(ret);
    }
}

int munmap(void* addr, size_t size) {
    int ret = syscall(SYS_munmap, reinterpret_cast<uintptr_t>(addr), size);
    __set_errno_return(ret, 0, -1);
}

}