#include <kernel/devices/storage/ide/controller.h>
#include <std/vector.h>

namespace kernel {

RefPtr<IDEController> IDEController::create() {
    pci::Address address;
    pci::enumerate([&address](pci::Device device) {
        if (device.is_ide_controller()) {
            address = device.address;
        }
    });

    if (!address.value) {
        return nullptr;
    }

    return IDEController::create(address);
}

RefPtr<IDEController> IDEController::create(pci::Address address) {
    auto controller = RefPtr<IDEController>(new IDEController(address));
    controller->m_devices.fill({});

    controller->enumerate();
    return controller;
}

void IDEController::enumerate() {
    for (size_t channel = 0; channel < 2; channel++) {
        for (size_t drive = 0; drive < 2; drive++) {
            m_devices[channel * 2 + drive] = PATADevice::create(
                static_cast<ata::Channel>(channel), 
                static_cast<ata::Drive>(drive),
                m_address
            );
        }
    }
}

RefPtr<StorageDevice> IDEController::device(ata::Channel channel, ata::Drive drive) const {
    return m_devices[to_underlying(channel) * 2 + to_underlying(drive)];
}

RefPtr<StorageDevice> IDEController::device(size_t index) const {
    Array<RefPtr<StorageDevice>, 4> devices;
    size_t count = 0;

    for (const auto& device : m_devices) {
        if (device) {
            devices[count++] = device;
        }
    }

    if (index >= count) {
        return nullptr;
    }

    return devices[index];
}

size_t IDEController::devices() const {
    size_t count = 0;
    for (const auto& device : m_devices) {
        if (device) {
            count++;
        }
    }

    return count;
}

}