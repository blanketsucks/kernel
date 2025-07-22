#include <kernel/devices/gpu/virtio/connector.h>
#include <kernel/devices/gpu/virtio/device.h>
#include <kernel/memory/manager.h>

namespace kernel {

using namespace virtio;

VirtIOGPUConnector::VirtIOGPUConnector(
    VirtIOGPUDevice* device, u32 id, GPURect rect
) : GPUConnector(id), m_device(device), m_rect(rect) {}

ErrorOr<void> VirtIOGPUConnector::initialize() {
    m_resource_id = TRY(m_device->create_resource_2d(GPUFormat::B8G8R8X8, m_rect.width, m_rect.height));
    size_t size = m_rect.width * m_rect.height * sizeof(u32);

    m_framebuffer = TRY(MM->allocate_kernel_region(size));

    TRY(m_device->attach_resource_backing(m_resource_id, reinterpret_cast<VirtualAddress>(m_framebuffer), size));
    TRY(m_device->set_resource_scanout(m_id, m_resource_id, m_rect));

    u32* fb = reinterpret_cast<u32*>(m_framebuffer);
    memset(fb, 0x41, m_rect.width * m_rect.height * sizeof(u32));

    TRY(this->flush());
    return {};
}

ErrorOr<GPUConnector::Resolution> VirtIOGPUConnector::get_resolution() const {
    return GPUConnector::Resolution {
        static_cast<int>(m_rect.width),
        static_cast<int>(m_rect.height),
        static_cast<int>(m_rect.width * 4),
        32
    };
}

ErrorOr<void*> VirtIOGPUConnector::map_framebuffer(Process* process) {
    size_t size = m_rect.width * m_rect.height * sizeof(u32);
    return process->allocate_from_kernel_region(
        reinterpret_cast<VirtualAddress>(m_framebuffer), size, PROT_READ | PROT_WRITE
    );
}

ErrorOr<void> VirtIOGPUConnector::flush() {
    TRY(m_device->transfer_to_host_2d(m_resource_id, m_rect, 0));
    TRY(m_device->resource_flush(m_resource_id, m_rect));

    return {};
}

}