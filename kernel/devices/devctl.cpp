#include <kernel/devices/devctl.h>

namespace kernel {

RefPtr<DeviceControl> DeviceControl::create() {
    return RefPtr<DeviceControl>(new DeviceControl());
}

ErrorOr<size_t> DeviceControl::read(void* buffer, size_t size, size_t offset) {
    if ((size % sizeof(DeviceEvent)) != 0) {
        return Error(EINVAL);
    } else if (offset != 0) {
        return Error(EINVAL);
    }

    auto& queue = Device::event_queue();
    size_t nread = 0;

    for (size_t i = 0; i < size / sizeof(DeviceEvent); i++) {
        if (queue.empty()) {
            break;
        }

        auto event = queue.dequeue();
        memcpy(reinterpret_cast<u8*>(buffer) + nread, &event, sizeof(DeviceEvent));

        nread += sizeof(DeviceEvent);
    }

    return nread;
}

}