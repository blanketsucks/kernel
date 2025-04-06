#pragma once

#include <kernel/devices/storage/controller.h>

#include <std/vector.h>

namespace kernel {

class StorageManager {
public:
    StorageManager() = default;
    static StorageManager* instance();

    static void initialize();

    static u32 generate_device_minor();

private:

    void enumerate_controllers();
    void enumerate_devices(RefPtr<StorageController>);

    Vector<RefPtr<StorageController>> m_controllers;
    Vector<RefPtr<StorageDevice>> m_devices;
};

}