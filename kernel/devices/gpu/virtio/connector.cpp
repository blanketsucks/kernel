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
    size_t size = m_rect.width * m_rect.height * 4;

    m_framebuffer = MM->allocate_kernel_region(size);
    PhysicalAddress address = MM->get_physical_address(m_framebuffer);

    TRY(m_device->attach_resource_backing(m_resource_id, address, size));
    TRY(m_device->set_resource_scanout(m_id, m_resource_id, m_rect));

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
    size_t size = m_rect.width * m_rect.height * 4;
    return process->allocate_with_physical_region(
        MM->get_physical_address(m_framebuffer), size, PROT_READ | PROT_WRITE
    );
}

ErrorOr<void> VirtIOGPUConnector::flush() {
    TRY(m_device->transfer_to_host_2d(m_resource_id, m_rect, 0));
    TRY(m_device->resource_flush(m_resource_id, m_rect));
    
    return {};
}

}