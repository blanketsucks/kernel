#include <kernel/devices/gpu/generic/generic.h>
#include <kernel/boot/boot_info.h>
#include <kernel/process/process.h>
#include <kernel/process/scheduler.h>
#include <kernel/posix/sys/ioctl.h>

namespace kernel {

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
    PhysicalAddress framebuffer, u32 width, u32 height, u32 pitch, u16 bpp
) {
    return Device::create<GenericGPUDevice>(framebuffer, width, height, pitch, bpp);
}

ErrorOr<void*> GenericGPUDevice::mmap(Process& process, size_t size, int prot) {
    if (size < std::align_up(this->size(), PAGE_SIZE)) {
        return Error(EINVAL);
    }
    
    void* region = process.allocate_with_physical_region(m_framebuffer, size, prot);
    return region;
}

ErrorOr<int> GenericGPUDevice::ioctl(unsigned request, unsigned arg) {
    auto* process = Process::current();
    switch (request) {
        case FB_GET_RESOLUTION: {
            auto* resolution = reinterpret_cast<FrameBufferResolution*>(arg);
            process->validate_write(resolution, sizeof(FrameBufferResolution));

            resolution->width = m_width;
            resolution->height = m_height;
            resolution->pitch = m_pitch;

            return 0;
        }
    }

    return -EINVAL;
}

}