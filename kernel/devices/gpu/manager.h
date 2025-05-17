#pragma once

#include <kernel/common.h>
#include <kernel/pci/device.h>

#include <std/memory.h>
#include <std/vector.h>

namespace kernel {

class GPUDevice;

class GPUManager {
public:
    GPUManager() = default;
    static GPUManager* instance();

    static void initialize();

    static u32 generate_device_minor();

private:
    void enumerate();
    void try_initialize_device(pci::Device device);

    Vector<RefPtr<GPUDevice>> m_devices;
};

}