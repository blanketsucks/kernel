#include <kernel/devices/null.h>

namespace kernel {

NullDevice* NullDevice::create() {
    return new NullDevice();
}

ErrorOr<size_t> NullDevice::read(void*, size_t, size_t) {
    return 0;
}

ErrorOr<size_t> NullDevice::write(const void*, size_t size, size_t) {
    return size;
}

}
