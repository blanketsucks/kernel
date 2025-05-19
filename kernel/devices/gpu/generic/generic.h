#pragma once

#include <kernel/common.h>
#include <kernel/devices/gpu/device.h>
#include <std/format.h>

namespace kernel {

class GenericGPUDevice : public GPUDevice {
public:
    static RefPtr<GenericGPUDevice> create_from_boot();
    static RefPtr<GenericGPUDevice> create(PhysicalAddress framebuffer, u32 width, u32 height, u32 pitch, u16 bpp);

    ErrorOr<void*> mmap(Process&, size_t size, int prot) override;
    ErrorOr<int> ioctl(unsigned request, unsigned arg) override;

private:
    friend class Device;

    GenericGPUDevice(
        PhysicalAddress framebuffer, u32 width, u32 height, u32 pitch, u16 bpp
    ) : m_framebuffer(framebuffer), m_width(width), m_height(height), m_pitch(pitch), m_bpp(bpp) {}

    PhysicalAddress m_framebuffer;

    u32 m_width;
    u32 m_height;
    u32 m_pitch;
    u16 m_bpp;
};

}