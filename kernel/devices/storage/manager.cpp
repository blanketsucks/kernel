#include <kernel/devices/storage/manager.h>
#include <kernel/pci.h>

#include <kernel/devices/storage/ahci/controller.h>
#include <kernel/devices/storage/ide/controller.h>

namespace kernel {

static StorageManager s_instance;

static u32 s_storage_device_minor = 0;

StorageManager* StorageManager::instance() {
    return &s_instance;
}

void StorageManager::initialize() {
    s_instance.enumerate_controllers();
}

u32 StorageManager::generate_device_minor() {
    return s_storage_device_minor++;
}

void StorageManager::enumerate_controllers() {
    pci::enumerate([&](pci::Device device) {
        if (device.class_id != pci::DeviceClass::MassStorageController) {
            return;
        }

        RefPtr<StorageController> controller;
        switch (device.subclass_id) {
            case pci::DeviceSubclass::SATAController:
                // TODO: Check prog_if
                controller = AHCIController::create(device);
                break;
            case pci::DeviceSubclass::IDEController:
                controller = IDEController::create(device);
                break;
            default:
                return;
        }
        
        if (controller) {
            m_controllers.append(controller);
            this->enumerate_devices(controller);
        }
    });
}

void StorageManager::enumerate_devices(RefPtr<StorageController> controller) {
    for (size_t i = 0; i < controller->devices(); i++) {
        auto device = controller->device(i);
        if (device) {
            m_devices.append(device);
        }
    }
}

}