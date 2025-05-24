#pragma once

#include <kernel/common.h>
#include <kernel/pci/device.h>

#include <std/function.h>

namespace kernel::pci {

class Controller {
public:
    virtual ~Controller() = default;

    u32 domain() const { return m_domain; }

    virtual u8 read8(u8 bus, u8 device, u8 function, u32 offset) = 0;
    virtual u16 read16(u8 bus, u8 device, u8 function, u32 offset) = 0;
    virtual u32 read32(u8 bus, u8 device, u8 function, u32 offset) = 0;

    virtual void write8(u8 bus, u8 device, u8 function, u32 offset, u8 value) = 0;
    virtual void write16(u8 bus, u8 device, u8 function, u32 offset, u16 value) = 0;
    virtual void write32(u8 bus, u8 device, u8 function, u32 offset, u32 value) = 0;

    void enumerate(Function<void(const Device&)>&& callback);

protected:
    Controller(u32 domain) : m_domain(domain) {}

private:
    void enumerate_bus(u8 bus, Function<void(const Device&)>& callback);
    void enumerate_slot(u8 bus, u8 slot, Function<void(const Device&)>& callback);
    void enumerate_functions(u8 bus, u8 slot, u8 function, Function<void(const Device&)>& callback);

    u32 m_domain;
};

}