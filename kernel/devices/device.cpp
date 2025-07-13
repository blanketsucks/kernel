#include <kernel/devices/device.h>
#include <kernel/fs/fd.h>

#include <std/format.h>

namespace kernel {

static HashMap<u32, RefPtr<Device>> s_devices;
static Queue<DeviceEvent> s_event_queue;

Device::Device(DeviceMajor major, u32 minor) : m_major(major), m_minor(minor) {}

void Device::add_device_event(Device const& device, DeviceEvent::Type type) {
    DeviceEvent event = { to_underlying(device.major()), device.minor(), type, device.is_block_device() };
    s_event_queue.enqueue(event);
}

void Device::after_device_creation(RefPtr<Device> device) {
    add_device_event(*device, DeviceEvent::Added);
    dev_t dev = encode(to_underlying(device->major()), device->minor());

    if (s_devices.contains(dev)) {
        dbgln("Device {}:{} already exists.", (int)device->major(), device->minor());
        return;
    }

    s_devices.set(dev, device);
}

void Device::remove() {
    add_device_event(*this, DeviceEvent::Removed);
    dev_t dev = encode(to_underlying(m_major), m_minor);

    if (!s_devices.contains(dev)) {
        dbgln("Device {}:{} does not exist.", (int)m_major, m_minor);
        return;
    }

    s_devices.remove(dev);
}

Queue<DeviceEvent>& Device::event_queue() {
    return s_event_queue;
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