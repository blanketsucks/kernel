#pragma once

#include <kernel/common.h>
#include <kernel/devices/block_device.h>
#include <kernel/posix/errno.h>

namespace kernel::devices {

constexpr u16 MAX_RESOLUTION_WIDTH = 4096;
constexpr u16 MAX_RESOLUTION_HEIGHT = 2160;

constexpr u16 VBE_IOPORT_INDEX = 0x01CE;
constexpr u16 VBE_IOPORT_DATA = 0x01CF;

constexpr u16 VBE_BPP_4  = 0x04;
constexpr u16 VBE_BPP_8  = 0x08;
constexpr u16 VBE_BPP_15 = 0x0F;
constexpr u16 VBE_BPP_16 = 0x10;
constexpr u16 VBE_BPP_24 = 0x18;
constexpr u16 VBE_BPP_32 = 0x20;

constexpr u16 VBE_INDEX_ID = 0x0;
constexpr u16 VBE_INDEX_XRES = 0x1;
constexpr u16 VBE_INDEX_YRES = 0x2;
constexpr u16 VBE_INDEX_BPP = 0x3;
constexpr u16 VBE_INDEX_ENABLE = 0x4;
constexpr u16 VBE_INDEX_BANK = 0x5;
constexpr u16 VBE_INDEX_VIRT_WIDTH = 0x6;
constexpr u16 VBE_INDEX_VIRT_HEIGHT = 0x7;
constexpr u16 VBE_INDEX_X_OFFSET = 0x8;
constexpr u16 VBE_INDEX_Y_OFFSET = 0x9;

constexpr u16 VBE_DISABLED = 0x00;
constexpr u16 VBE_ENABLED = 0x01;
constexpr u16 VBE_LFB_ENABLED = 0x40;

class BochsVGADevice : public BlockDevice {
public:
    static constexpr u16 VENDOR_ID = 0x1234;
    static constexpr u16 DEVICE_ID = 0x1111;

    BochsVGADevice() : BlockDevice(29, 0, 0) {}

    static BochsVGADevice* create(i32 width, i32 height);
    static BochsVGADevice* instance();

    bool read_blocks(void*, size_t, size_t) override { return -EINVAL; }
    bool write_blocks(const void*, size_t, size_t) override { return -EINVAL; }

    u32* framebuffer() const { return m_framebuffer; }
    u32 physical_address() const { return m_physical_address; }

    i32 width() const { return m_width; }
    i32 height() const { return m_height; }
    i32 bpp() const { return m_bpp; }

    size_t size() const override { return m_width * m_height * (m_bpp / 8); }

    void write_register(u16 index, u16 value);
    u16 read_register(u16 index);

    void set_resolution(i32 width, i32 height, i32 bpp, bool map = true);

    bool map();
    bool remap();

    void set_pixel(i32 x, i32 y, u32 color);

    ErrorOr<void*> mmap(Process&, size_t size, int prot) override;
    ErrorOr<int> ioctl(unsigned request, unsigned arg) override;
    
private:
    static BochsVGADevice* s_instance;

    // Converts a value like 32 to the equivalent VBE BPP value which is 0x20 in this case.
    static i32 bpp_to_vbe_bpp(i32 bpp);

    u32 m_physical_address;
    u32* m_framebuffer = nullptr;

    i32 m_height;
    i32 m_width;
    i32 m_bpp;
};

}