#include <kernel/pci/pci.h>
#include <kernel/pci/address.h>
#include <kernel/pci/device.h>
#include <kernel/pci/controllers/piix4.h>
#include <kernel/pci/controllers/mmio.h>
#include <kernel/acpi/acpi.h>

#include <std/format.h>

namespace kernel {

static PCI s_instance;

PCI* PCI::instance() {
    return &s_instance;
}

void PCI::initialize() {
    s_instance.init();
}

void PCI::enumerate(Function<void(const pci::Device&)>&& callback) {
    for (const auto& device : s_instance.m_devices) {
        callback(device);
    }
}

Optional<pci::Device> PCI::find_device(u16 vendor_id, u16 device_id, i16 prog_if) {
    for (auto& device : s_instance.m_devices) {
        if (device.vendor_id() != vendor_id || device.device_id() != device_id) {
            continue;
        }

        if (prog_if == -1 || device.address().prog_if() == prog_if) {
            return device;
        }
    }

    return None;
}

void PCI::init() {
    auto* parser = acpi::Parser::instance();
    parser->init();

    auto* mcfg = parser->find_table<acpi::MCFG>("MCFG");
    if (!mcfg) {
        auto controller = pci::PIIX4Controller::create();
        controller->enumerate([this](const pci::Device& device) {
            m_devices.append(device);
        });
        
        m_controllers.append(move(controller));
        return;
    }

    size_t entries = (mcfg->header.length - sizeof(acpi::SDTHeader)) / sizeof(mcfg->entries[0]);
    for (size_t i = 0; i < entries; i++) {
        auto controller = pci::MMIOController::create(i, mcfg->entries[i].base_address);

        controller->enumerate([this](const pci::Device& device) {
            m_devices.append(device);
        });

        m_controllers.append(move(controller));
    }
}

template<> u8 PCI::read<u8>(u32 domain, u8 bus, u8 device, u8 function, u32 offset) {
    auto controller = s_instance.m_controllers[domain];    
    return controller->read8(bus, device, function, offset);
}

template<> u16 PCI::read<u16>(u32 domain, u8 bus, u8 device, u8 function, u32 offset) {
    auto controller = s_instance.m_controllers[domain];
    return controller->read16(bus, device, function, offset);
}

template<> u32 PCI::read<u32>(u32 domain, u8 bus, u8 device, u8 function, u32 offset) {
    auto controller = s_instance.m_controllers[domain];
    return controller->read32(bus, device, function, offset);
}

template<> void PCI::write<u8>(u32 domain, u8 bus, u8 device, u8 function, u32 offset, u8 value) {
    auto controller = s_instance.m_controllers[domain];
    controller->write8(bus, device, function, offset, value);
}

template<> void PCI::write<u16>(u32 domain, u8 bus, u8 device, u8 function, u32 offset, u16 value) {
    auto controller = s_instance.m_controllers[domain];
    controller->write16(bus, device, function, offset, value);
}

template<> void PCI::write<u32>(u32 domain, u8 bus, u8 device, u8 function, u32 offset, u32 value) {
    auto controller = s_instance.m_controllers[domain];
    controller->write32(bus, device, function, offset, value);
}

}