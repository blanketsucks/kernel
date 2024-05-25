#include <kernel/devices/device.h>
#include <kernel/fs/fd.h>

namespace kernel {

static HashMap<u32, Device*> s_devices;

Device::Device(u32 major, u32 minor) : m_major(major), m_minor(minor) {
    s_devices.set(encode(major, minor), this);
}

Device* Device::get_device(u32 major, u32 minor) {
    auto iterator = s_devices.find(encode(major, minor));
    if (iterator == s_devices.end()) {
        return nullptr;
    }

    return iterator->value;
}

RefPtr<fs::FileDescriptor> Device::open(int options) {
    return fs::FileDescriptor::create(this, options);
}

}