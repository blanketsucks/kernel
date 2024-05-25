#include <kernel/devices/null.h>

namespace kernel::devices {

ssize_t NullDevice::read(void*, size_t, size_t) {
    return 0;
}

ssize_t NullDevice::write(const void*, size_t size, size_t) {
    return size;
}

}
