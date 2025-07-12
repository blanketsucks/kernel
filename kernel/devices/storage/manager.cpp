#include <kernel/devices/storage/manager.h>
#include <kernel/boot/command_line.h>
#include <kernel/fs/devfs/filesystem.h>
#include <kernel/pci/pci.h>

#include <kernel/devices/storage/ahci/controller.h>
#include <kernel/devices/storage/ide/controller.h>

#include <std/format.h>

namespace kernel {

static StorageManager s_instance;

static u32 s_storage_device_minor = 0;
static u32 s_partition_device_minor = 0;

static constexpr StringView s_pata_device_prefix = "hd";
static constexpr StringView s_sata_device_prefix = "sd";

StorageManager* StorageManager::instance() {
    return &s_instance;
}

void StorageManager::initialize() {
    devfs::register_device_range("hd", DeviceMajor::Storage, devfs::FormatStyle::ASCII);
    devfs::register_device_range(DeviceMajor::StoragePartition, [](DeviceEvent event) -> String {
        // FIXME: Maybe we could just pass in the device pointer directly to DeviceEvent. And maybe we could find a whole better way
        //        to do this kind of thing in the first place.
        RefPtr<StorageDevicePartition> partition = Device::get_device(static_cast<DeviceMajor>(event.major), event.minor);
        size_t index = partition->partition().index + 1;

        auto* device = partition->device();
        StringView path = devfs::get_device_path(static_cast<u32>(device->major()), device->minor());

        return format("{}{}", path, index);
    });
    
    s_instance.enumerate_controllers();

}

u32 StorageManager::generate_device_minor() {
    return s_storage_device_minor++;
}

u32 StorageManager::generate_partition_minor() {
    return s_partition_device_minor++;
}

void StorageManager::enumerate_controllers() {
    PCI::enumerate([&](pci::Device device) {
        if (device.class_id() != pci::DeviceClass::MassStorageController) {
            return;
        }

        RefPtr<StorageController> controller;
        switch (device.subclass_id()) {
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
        if (!device) {
            continue;
        }

        this->enumerate_device_partitions(&*device);
        m_devices.append(device);
    }
}

void StorageManager::enumerate_device_partitions(StorageDevice* device) {
    Vector<PartitionEntry> partitions = ::kernel::enumerate_device_partitions(device);
    for (auto& partition : partitions) {
        auto partition_device = StorageDevicePartition::create(device, partition);
        device->m_partitions.append(partition_device);
    }
}

StorageManager::BootDevice StorageManager::parse_boot_device(StringView device) {
    // TODO: Find a better, cleaner way to format the boot device to make parsing easier
    //       Something like "ide:0:0" to indicate the first IDE device, or "ahci:0:0" for the first AHCI device
    //       and "ide:0:1" for the first partition on the first IDE device, and so on.
    if (device.startswith("/dev/")) {
        device = device.substr(5);
    }

    StorageDevice::Type type;
    if (device.startswith(s_pata_device_prefix)) {
        type = StorageDevice::PATA;
    } else if (device.startswith(s_sata_device_prefix)) {
        type = StorageDevice::SATA;
    } else {
        return {};
    }

    device = device.substr(2);
    u32 index = 0;

    // a is the first device, b is the second, etc.
    if (device[0] >= 'a' && device[0] <= 'z') {
        index = device[0] - 'a';
        device = device.substr(1);
    } else {
        return {};
    }

    if (device.empty()) {
        return { static_cast<u8>(type), index, 0 };
    }

    u32 partition = 0;
    if (std::isdigit(device[0])) {
        partition = (device[0] - '0');
        device = device.substr(1);
    }

    if (!device.empty()) {
        return {};
    }

    return { static_cast<u8>(type), index, partition };
}

BlockDevice* StorageManager::determine_boot_device() {
    auto* cmdline = CommandLine::instance();
    StringView root = cmdline->root();

    if (root.empty()) {
        return nullptr;
    }

    auto boot_device = s_instance.parse_boot_device(root);
    size_t index = 0;

    for (auto& device : s_instance.m_devices) {
        if (device->type() != boot_device.type) {
            continue;
        }
        
        if (index != boot_device.index) {
            index++;
            continue;
        }

        if (boot_device.partition) {
            return device->partition(boot_device.partition - 1).ptr();
        } else {
            return device.ptr();
        }
    }

    return nullptr;
}



}