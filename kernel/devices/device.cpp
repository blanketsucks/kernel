#include <kernel/devices/device.h>
#include <kernel/fs/fd.h>

namespace kernel {

static HashMap<u32, RefPtr<Device>> s_devices;

Device::Device(DeviceMajor major, u32 minor) : m_major(major), m_minor(minor) {
    s_devices.set(encode(to_underlying(major), minor), this);
}

RefPtr<Device> Device::get_device(DeviceMajor major, u32 minor) {
    auto iterator = s_devices.find(encode(to_underlying(major), minor));
    if (iterator == s_devices.end()) {
        return nullptr;
    }

    return iterator->value;
}

RefPtr<Device> Device::as_ref() const {
    return Device::get_device(m_major, m_minor);
}

RefPtr<fs::FileDescriptor> Device::open(int options) {
    return fs::FileDescriptor::create(as_ref(), options);
}

}