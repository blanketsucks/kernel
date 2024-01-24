#include <kernel/devices/bochs_vga.h>
#include <kernel/pci.h>
#include <kernel/io.h>
#include <kernel/serial.h>
#include <kernel/memory/mm.h>

namespace kernel::devices {

BochsVGADevice* BochsVGADevice::s_instance = nullptr;

BochsVGADevice* BochsVGADevice::create(i32 width, i32 height) {
    pci::Address address = {};
    pci::enumerate([&address](pci::Device device) {
        if (device.vendor_id == VENDOR_ID && device.id == DEVICE_ID) {
            address = device.address;
        }
    });

    if (!address.value) {
        return nullptr;
    }

    auto* device = new BochsVGADevice();
    device->m_physical_address = address.bar0() & 0xFFFFFFF0;

    device->set_resolution(width, height, VBE_BPP_32);
    device->mmap();

    s_instance = device;
    return device;
}

BochsVGADevice* BochsVGADevice::instance() {
    return s_instance;
}

void BochsVGADevice::write_register(u16 index, u16 value) {
    io::write<u16>(VBE_IOPORT_INDEX, index);
    io::write<u16>(VBE_IOPORT_DATA, value);
}

u16 BochsVGADevice::read_register(u16 index) {
    io::write<u16>(VBE_IOPORT_INDEX, index);
    return io::read<u16>(VBE_IOPORT_DATA);
}

void BochsVGADevice::set_resolution(i32 width, i32 height, i32 bpp) {
    if (width > MAX_RESOLUTION_WIDTH || height > MAX_RESOLUTION_HEIGHT) {
        return;
    }

    this->write_register(VBE_INDEX_ENABLE, VBE_DISABLED);
    this->write_register(VBE_INDEX_XRES, width);
    this->write_register(VBE_INDEX_YRES, height);
    this->write_register(VBE_INDEX_VIRT_WIDTH, width);
    this->write_register(VBE_INDEX_VIRT_HEIGHT, height * 2);
    this->write_register(VBE_INDEX_BPP, bpp);
    this->write_register(VBE_INDEX_ENABLE, VBE_ENABLED | VBE_LFB_ENABLED);
    this->write_register(VBE_INDEX_BANK, 0);

    m_width = width;
    m_height = height;
    m_bpp = bpp;
}

bool BochsVGADevice::mmap() {
    auto* mm = memory::MemoryManager::instance();
    auto result = mm->map_physical_region(m_physical_address, this->size());

    if (result.is_err()) {
        return false;
    }

    m_framebuffer = reinterpret_cast<u32*>(result.value());
    serial::printf("end = 0x%x\n", u32(m_framebuffer + this->size()));
    return true;
}

void BochsVGADevice::set_pixel(i32 x, i32 y, u32 color) {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        return;
    }

    m_framebuffer[y * m_width + x] = color;
}

}