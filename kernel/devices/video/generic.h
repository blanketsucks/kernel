#pragma once

#include <kernel/common.h>
#include <kernel/devices/block_device.h>

namespace kernel {

class GenericVideoDevice : public BlockDevice {
public:
    static GenericVideoDevice* create_from_boot();
    static GenericVideoDevice* create(PhysicalAddress framebuffer, u32 width, u32 height, u32 pitch, u16 bpp);

    size_t max_io_block_count() const override { return 0; }
    bool read_blocks(void*, size_t, size_t) override { return -EINVAL; }
    bool write_blocks(const void*, size_t, size_t) override { return -EINVAL; }

    ErrorOr<void*> mmap(Process&, size_t size, int prot) override;
    ErrorOr<int> ioctl(unsigned request, unsigned arg) override;

private:
    GenericVideoDevice(
        PhysicalAddress framebuffer, u32 width, u32 height, u32 pitch, u16 bpp
    ) : BlockDevice(DeviceMajor::Video, 0, 0), 
        m_framebuffer(framebuffer), m_width(width), m_height(height), m_pitch(pitch), m_bpp(bpp) {}

    PhysicalAddress m_framebuffer;

    u32 m_width;
    u32 m_height;
    u32 m_pitch;
    u16 m_bpp;
};

}