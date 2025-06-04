#pragma once

#include <kernel/devices/gpu/virtio/virtio.h>
#include <kernel/devices/gpu/device.h>

namespace kernel {

class VirtIOGPUDevice;

class VirtIOGPUConnector : public GPUConnector {
public:
    static RefPtr<VirtIOGPUConnector> create(VirtIOGPUDevice* device, u32 id, virtio::GPURect rect) {
        return RefPtr<VirtIOGPUConnector>(new VirtIOGPUConnector(device, id, rect));
    }

    ErrorOr<void> initialize();

    ErrorOr<Resolution> get_resolution() const override;
    ErrorOr<void*> map_framebuffer(Process* process) override;
    ErrorOr<void> flush() override;

private:
    VirtIOGPUConnector(VirtIOGPUDevice* device, u32 id, virtio::GPURect rect);

    VirtIOGPUDevice* m_device;
    virtio::GPURect m_rect;

    u32 m_resource_id = 0;
    void* m_framebuffer = nullptr;
};

}