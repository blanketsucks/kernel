#include <kernel/devices/gpu/generic/generic.h>
#include <kernel/boot/boot_info.h>
#include <kernel/process/process.h>
#include <kernel/process/scheduler.h>
#include <kernel/posix/sys/ioctl.h>

namespace kernel {

ErrorOr<GPUConnector::Resolution> GenericGPUConnector::get_resolution() const {
    return GPUConnector::Resolution {
        m_device->width(),
        m_device->height(),
        m_device->width() * (m_device->bpp() / 8),
        m_device->bpp()
    };
}

ErrorOr<void*> GenericGPUConnector::map_framebuffer(Process* process) {
    PhysicalAddress address = m_device->framebuffer();
    size_t size = m_device->height() * m_device->pitch();

    return process->allocate_with_physical_region(address, size, PROT_READ | PROT_WRITE);
}

ErrorOr<void> GenericGPUConnector::flush() {
    return {};
}

RefPtr<GenericGPUDevice> GenericGPUDevice::create_from_boot() {
    return GenericGPUDevice::create(
        reinterpret_cast<PhysicalAddress>(g_boot_info->framebuffer.address),
        g_boot_info->framebuffer.width,
        g_boot_info->framebuffer.height,
        g_boot_info->framebuffer.pitch,
        g_boot_info->framebuffer.bpp
    );
}

RefPtr<GenericGPUDevice> GenericGPUDevice::create(
    PhysicalAddress framebuffer, i32 width, i32 height, i32 pitch, i32 bpp
) {
    return Device::create<GenericGPUDevice>(framebuffer, width, height, pitch, bpp);
}

GenericGPUDevice::GenericGPUDevice(
    PhysicalAddress framebuffer, i32 width, i32 height, i32 pitch, i32 bpp
) : m_framebuffer(framebuffer), m_width(width), m_height(height), m_pitch(pitch), m_bpp(bpp) {
    m_connectors.append(GenericGPUConnector::create(this));
}

}