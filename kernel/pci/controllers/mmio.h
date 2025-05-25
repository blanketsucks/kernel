#pragma once

#include <kernel/pci/controllers/controller.h>

#include <std/array.h>
#include <std/memory.h>

namespace kernel::pci {

class MMIOController : public Controller {
public:
    static RefPtr<MMIOController> create(u32 domain, PhysicalAddress base_address);

    static constexpr size_t MAX_BUS_COUNT = 256;
    static constexpr size_t MAX_DEVICE_COUNT = 32;
    static constexpr size_t MAX_FUNCTION_COUNT = 8;

    static constexpr size_t CONFIGURATION_SPACE_SIZE = 4096;

    static constexpr size_t BUS_SIZE = MAX_DEVICE_COUNT * MAX_FUNCTION_COUNT * CONFIGURATION_SPACE_SIZE;

    u8 read8(u8 bus, u8 device, u8 function, u32 offset) override;
    u16 read16(u8 bus, u8 device, u8 function, u32 offset) override;
    u32 read32(u8 bus, u8 device, u8 function, u32 offset) override;

    void write8(u8 bus, u8 device, u8 function, u32 offset, u8 value) override;
    void write16(u8 bus, u8 device, u8 function, u32 offset, u16 value) override;
    void write32(u8 bus, u8 device, u8 function, u32 offset, u32 value) override;

private:
    MMIOController(u32 domain, PhysicalAddress base_address) : Controller(domain), m_base_address(base_address) {
        for (size_t i = 0; i < MAX_BUS_COUNT; ++i) {
            m_mapped_buses[i] = nullptr;
        }
    }

    PhysicalAddress get_bus_base_address(u8 bus) const;
    u8* get_device_configuration_space(u8 bus, u8 device, u8 function);

    PhysicalAddress m_base_address;
    Array<u8*, MAX_BUS_COUNT> m_mapped_buses; // TODO: Do we really need to store all of them?

    u8* map_bus(u8 bus);
};


}