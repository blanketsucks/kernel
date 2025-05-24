#include <kernel/pci/controllers/mmio.h>
#include <kernel/memory/manager.h>

namespace kernel::pci {
    
RefPtr<MMIOController> MMIOController::create(u32 domain, PhysicalAddress base_address) {
    return RefPtr<MMIOController>(new MMIOController(domain, base_address));
}

PhysicalAddress MMIOController::get_bus_base_address(u8 bus) const {
    return m_base_address + (BUS_SIZE * bus);
}

u8* MMIOController::map_bus(u8 bus) {
    if (!m_mapped_buses[bus]) {
        PhysicalAddress bus_base = this->get_bus_base_address(bus);
        m_mapped_buses[bus] = reinterpret_cast<u8*>(
            MM->map_physical_region(reinterpret_cast<void*>(bus_base), BUS_SIZE)
        );
    }

    return m_mapped_buses[bus];
}

u8* MMIOController::get_device_configuration_space(u8 bus, u8 device, u8 function) {
    u8* bus_base = this->map_bus(bus);
    return bus_base + ((device * MAX_FUNCTION_COUNT + function) * CONFIGURATION_SPACE_SIZE);
}

u8 MMIOController::read8(u8 bus, u8 device, u8 function, u32 offset) {
    u8* config_space = this->get_device_configuration_space(bus, device, function);
    return *(reinterpret_cast<u8 volatile*>(config_space + offset));
}

u16 MMIOController::read16(u8 bus, u8 device, u8 function, u32 offset) {
    u8* config_space = this->get_device_configuration_space(bus, device, function);
    return *(reinterpret_cast<u16 volatile*>(config_space + offset));
}

u32 MMIOController::read32(u8 bus, u8 device, u8 function, u32 offset) {
    u8* config_space = this->get_device_configuration_space(bus, device, function);
    return *(reinterpret_cast<u32 volatile*>(config_space + offset));
}

void MMIOController::write8(u8 bus, u8 device, u8 function, u32 offset, u8 value) {
    u8* config_space = this->get_device_configuration_space(bus, device, function);
    *(reinterpret_cast<u8 volatile*>(config_space + offset)) = value;
}

void MMIOController::write16(u8 bus, u8 device, u8 function, u32 offset, u16 value) {
    u8* config_space = this->get_device_configuration_space(bus, device, function);
    *(reinterpret_cast<u16 volatile*>(config_space + offset)) = value;
}

void MMIOController::write32(u8 bus, u8 device, u8 function, u32 offset, u32 value) {
    u8* config_space = this->get_device_configuration_space(bus, device, function);
    *(reinterpret_cast<u32 volatile*>(config_space + offset)) = value;
}



}