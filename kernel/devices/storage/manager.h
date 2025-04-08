#pragma once

#include <kernel/devices/storage/controller.h>
#include <kernel/devices/block_device.h>

#include <std/vector.h>

namespace kernel {

class StorageManager {
public:
    StorageManager() = default;
    static StorageManager* instance();

    static void initialize();

    static u32 generate_device_minor();
    static u32 generate_partition_minor();

    static BlockDevice* determine_boot_device();

private:
    struct BootDevice {
        u8 type;
        u32 index;
        u32 partition;
    };  

    void enumerate_controllers();
    void enumerate_devices(RefPtr<StorageController>);

    void enumerate_device_partitions(StorageDevice*);

    BootDevice parse_boot_device(StringView);

    Vector<RefPtr<StorageController>> m_controllers;
    Vector<RefPtr<StorageDevice>> m_devices;
};

}