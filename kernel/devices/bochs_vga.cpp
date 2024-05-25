#include <kernel/devices/bochs_vga.h>
#include <kernel/posix/sys/ioctl.h>
#include <kernel/process/process.h>
#include <kernel/memory/manager.h>
#include <kernel/process/process.h>
#include <kernel/process/scheduler.h>
#include <kernel/serial.h>
#include <kernel/pci.h>
#include <kernel/io.h>

namespace kernel::devices {

BochsVGADevice* BochsVGADevice::s_instance = nullptr;

i32 BochsVGADevice::bpp_to_vbe_bpp(i32 bpp) {
    switch (bpp) {
        case 8:
            return VBE_BPP_8;
        case 15:
            return VBE_BPP_15;
        case 16:
            return VBE_BPP_16;
        case 24:
            return VBE_BPP_24;
        case 32:
            return VBE_BPP_32;
        default:
            return VBE_BPP_32;
    }
}

BochsVGADevice* BochsVGADevice::create(i32 width, i32 height) {
    pci::Address address = {};
    pci::enumerate([&address](pci::Device device) {
        if (device.is_bochs_vga()) {
            address = device.address;
        }
    });

    if (!address.value) {
        return nullptr;
    }

    auto* device = new BochsVGADevice();
    device->m_physical_address = address.bar0() & 0xFFFFFFF0;

    device->set_resolution(width, height, 32, false);
    if (!device->map()) {
        return nullptr;
    }

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

void BochsVGADevice::set_resolution(i32 width, i32 height, i32 bpp, bool map) {
    if (width > MAX_RESOLUTION_WIDTH || height > MAX_RESOLUTION_HEIGHT) {
        return;
    }

    this->write_register(VBE_INDEX_ENABLE, VBE_DISABLED);
    this->write_register(VBE_INDEX_XRES, width);
    this->write_register(VBE_INDEX_YRES, height);
    this->write_register(VBE_INDEX_VIRT_WIDTH, width);
    this->write_register(VBE_INDEX_VIRT_HEIGHT, height * 2);
    this->write_register(VBE_INDEX_BPP, bpp_to_vbe_bpp(bpp));
    this->write_register(VBE_INDEX_ENABLE, VBE_ENABLED | VBE_LFB_ENABLED);
    this->write_register(VBE_INDEX_BANK, 0);

    m_width = width;
    m_height = height;
    m_bpp = bpp;

    if (map) {
        this->remap();
    }
}

bool BochsVGADevice::map() {
    void* framebuffer = MM->map_physical_region(m_physical_address, this->size());
    if (!framebuffer) {
        return false;
    }

    m_framebuffer = reinterpret_cast<u32*>(framebuffer);
    return true;
}

bool BochsVGADevice::remap() {
    // We just mark the old framebuffer as free for use and call map again.
    if (m_framebuffer) {
        auto& allocator = MM->kernel_region_allocator();
        allocator.free(reinterpret_cast<VirtualAddress>(m_framebuffer));
    }

    return this->map();
}

ErrorOr<void*> BochsVGADevice::mmap(Process& process, size_t size, int prot) {
    if (size < std::align_up(this->size(), PAGE_SIZE)) {
        return Error(EINVAL);
    }
    
    void* region = process.allocate_with_physical_region(m_physical_address, size, prot);
    return region;
}

void BochsVGADevice::set_pixel(i32 x, i32 y, u32 color) {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        return;
    }

    m_framebuffer[y * m_width + x] = color;
}

ErrorOr<int> BochsVGADevice::ioctl(unsigned request, unsigned arg) {
    auto* process = Scheduler::current_process();
    switch (request) {
        case FB_GET_RESOLUTION: {
            auto* resolution = reinterpret_cast<FrameBufferResolution*>(arg);
            process->validate_write(resolution, sizeof(FrameBufferResolution));

            resolution->width = m_width;
            resolution->height = m_height;
            resolution->pitch = m_width * (m_bpp / 8);

            return 0;
        }
        default:
            return Error(EINVAL);
    }
}

}