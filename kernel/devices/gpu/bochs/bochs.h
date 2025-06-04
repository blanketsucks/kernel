#pragma once

#include <kernel/common.h>
#include <kernel/devices/gpu/device.h>
#include <kernel/posix/errno.h>
#include <kernel/pci/pci.h>

namespace kernel {

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

constexpr u16 VBE_DISABLED = 0x00;
constexpr u16 VBE_ENABLED = 0x01;
constexpr u16 VBE_LFB_ENABLED = 0x40;

class BochsGPUDevice;

class BochsGPUConnector : public GPUConnector {
public:
    static RefPtr<BochsGPUConnector> create(BochsGPUDevice* device) {
        return RefPtr<BochsGPUConnector>(new BochsGPUConnector(device));
    }

    ErrorOr<Resolution> get_resolution() const override;
    ErrorOr<void*> map_framebuffer(Process*) override;
    ErrorOr<void> flush() override;

private:
    BochsGPUConnector(BochsGPUDevice* device) : GPUConnector(0), m_device(device) {}

    BochsGPUDevice* m_device;
};

class BochsGPUDevice : public GPUDevice {
public:
    enum Port : u16 {
        Index = 0x01CE,
        Data  = 0x01CF
    };

    enum Register : u16 {
        ID         = 0x0,
        XRes       = 0x1,
        YRes       = 0x2,
        BPP        = 0x3,
        Enable     = 0x4,
        Bank       = 0x5,
        VirtWidth  = 0x6,
        VirtHeight = 0x7,
        XOffset    = 0x8,
        YOffset    = 0x9
    };

    static constexpr u16 VENDOR_ID = 0x1234;
    static constexpr u16 DEVICE_ID = 0x1111;

    static RefPtr<GPUDevice> create(pci::Device);

    u32* framebuffer() const { return m_framebuffer; }
    PhysicalAddress physical_address() const { return m_physical_address; }

    i32 width() const { return m_width; }
    i32 height() const { return m_height; }
    i32 bpp() const { return m_bpp; }

    size_t size() const override { return m_width * m_height * (m_bpp / 8); }
    size_t max_io_block_count() const override { return 1; }

    void write_register(u16 index, u16 value);
    u16 read_register(u16 index);

    void set_resolution(i32 width, i32 height, i32 bpp, bool map = true);

    bool map();
    bool remap();
    
private:
    friend class Device;

    BochsGPUDevice(pci::Address);

    PhysicalAddress m_physical_address;
    u32* m_framebuffer = nullptr;

    i32 m_height;
    i32 m_width;
    i32 m_bpp;
};

}