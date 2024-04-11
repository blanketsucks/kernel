#include <kernel/devices/device.h>

namespace kernel::devices {

DeviceManager* DeviceManager::s_instance = nullptr;

DeviceManager::DeviceManager() {
    
}

void DeviceManager::init() {
    s_instance = new DeviceManager();
}

DeviceManager* DeviceManager::instance() {
    return s_instance;
}

bool DeviceManager::register_device(Device* device) {
    u32 encoded = Device::encode(device->major(), device->minor());
    if (m_devices.contains(encoded)) {
        return false;
    }

    m_devices.set(encoded, device);
    return true;
}

RefPtr<Device> DeviceManager::get_device(u32 major, u32 minor) {
    auto iterator = m_devices.find(Device::encode(major, minor));
    if (iterator == m_devices.end()) {
        return nullptr;
    }

    return iterator->value;
}

}