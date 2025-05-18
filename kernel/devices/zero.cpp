#include <kernel/devices/zero.h>
#include <std/string.h>

namespace kernel {

ZeroDevice* ZeroDevice::create() {
    return new ZeroDevice();
}

ErrorOr<size_t> ZeroDevice::read(void* buffer, size_t size, size_t) {
    memset(buffer, 0, size);
    return size;
}

ErrorOr<size_t> ZeroDevice::write(const void*, size_t size, size_t) {
    return size;
}

}