#include <kernel/devices/gpu/manager.h>
#include <kernel/devices/gpu/device.h>
#include <kernel/fs/devfs/filesystem.h>
#include <kernel/boot/boot_info.h>
#include <kernel/pci/pci.h>

#include <kernel/devices/gpu/virtio/device.h>
#include <kernel/devices/gpu/generic/generic.h>
#include <kernel/devices/gpu/bochs/bochs.h>

namespace kernel {

using GPUDeviceInitializer = RefPtr<GPUDevice> (*)(pci::Device);

static const Vector<GPUDeviceInitializer> s_gpu_device_initializers = {
    VirtIOGPUDevice::create,
    BochsGPUDevice::create,
};

static GPUManager s_instance;
static u32 s_device_minor = 0;

GPUManager* GPUManager::instance() {
    return &s_instance;
}

void GPUManager::initialize() {
    devfs::register_device_range("fb", DeviceMajor::GPU);

    s_instance.enumerate();
}

u32 GPUManager::generate_device_minor() {
    return s_device_minor++;
}

void GPUManager::enumerate() {
    if (g_boot_info->framebuffer.address) {
        m_devices.append(GenericGPUDevice::create_from_boot());
    }

    pci::enumerate([this](pci::Device device) {
        if (!device.is_display_controller()) {
            return;
        }

        this->try_initialize_device(device);
    });
}

void GPUManager::try_initialize_device(pci::Device pci_device) {
    for (auto& initializer : s_gpu_device_initializers) {
        auto gpu = initializer(pci_device);
        if (!gpu) {
            continue;
        }

        m_devices.append(gpu);
    }
}

}