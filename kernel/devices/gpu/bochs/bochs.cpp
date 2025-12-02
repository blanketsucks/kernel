#include <kernel/devices/gpu/bochs/bochs.h>
#include <kernel/posix/sys/ioctl.h>
#include <kernel/process/process.h>
#include <kernel/memory/manager.h>
#include <kernel/process/process.h>
#include <kernel/process/scheduler.h>
#include <kernel/serial.h>
#include <kernel/pci/pci.h>
#include <kernel/arch/io.h>

namespace kernel {

ErrorOr<GPUConnector::Resolution> BochsGPUConnector::get_resolution() const {
    return GPUConnector::Resolution {
        m_device->width(),
        m_device->height(),
        m_device->width() * (m_device->bpp() / 8),
        m_device->bpp()
    };
}

ErrorOr<void*> BochsGPUConnector::map_framebuffer(Process* process) {
    PhysicalAddress address = m_device->physical_address();
    size_t size = std::align_up(m_device->size(), PAGE_SIZE);

    return process->allocate_with_physical_region(address, size, PROT_READ | PROT_WRITE);
}

ErrorOr<void> BochsGPUConnector::flush() {
    return {};
}

RefPtr<GPUDevice> BochsGPUDevice::create(pci::Device pci_device) {
    if (pci_device.vendor_id() != VENDOR_ID || pci_device.device_id() != DEVICE_ID) {
        return nullptr;
    }

    return Device::create<BochsGPUDevice>(pci_device.address());
}

BochsGPUDevice::BochsGPUDevice(pci::Address address) {
    m_physical_address = address.bar(0) & 0xfffffff0;

    this->set_resolution(DEFAULT_WIDTH, DEFAULT_HEIGHT, 32);
    m_connectors.append(BochsGPUConnector::create(this));
}

void BochsGPUDevice::write_register(u16 index, u16 value) {
    io::write<u16>(VBE_IOPORT_INDEX, index);
    io::write<u16>(VBE_IOPORT_DATA, value);
}

u16 BochsGPUDevice::read_register(u16 index) {
    io::write<u16>(VBE_IOPORT_INDEX, index);
    return io::read<u16>(VBE_IOPORT_DATA);
}

void BochsGPUDevice::set_resolution(i32 width, i32 height, i32 bpp) {
    if (width > MAX_RESOLUTION_WIDTH || height > MAX_RESOLUTION_HEIGHT) {
        return;
    }

    this->write_register(Enable, VBE_DISABLED);
    this->write_register(XRes, width);
    this->write_register(YRes, height);
    this->write_register(VirtWidth, width);
    this->write_register(VirtWidth, height * 2);
    this->write_register(BPP, bpp);
    this->write_register(Enable, VBE_ENABLED | VBE_LFB_ENABLED);
    this->write_register(Bank, 0);

    m_width = width;
    m_height = height;
    m_bpp = bpp;
}

}