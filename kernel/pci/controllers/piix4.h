#pragma once

#include <kernel/pci/controllers/controller.h>

namespace kernel::pci {

class PIIX4Controller : public Controller {
public:
    union IOAddress {
        struct {
            u32 register_offset : 8;
            u32 function : 3;
            u32 device : 5;
            u32 bus : 8;
            u32 reserved : 7;
            u32 enable : 1;
        };

        u32 value;

        IOAddress() : value(0) {}
        IOAddress(
            u8 bus, u8 device, u8 function
        ) : register_offset(0), function(function), device(device), bus(bus), reserved(0), enable(1) {}
    };

    static constexpr u16 CONFIG_ADDRESS = 0xCF8;
    static constexpr u16 CONFIG_DATA = 0xCFC;

    static RefPtr<PIIX4Controller> create();

    u8 read8(u8 bus, u8 device, u8 function, u32 offset) override;
    u16 read16(u8 bus, u8 device, u8 function, u32 offset) override;
    u32 read32(u8 bus, u8 device, u8 function, u32 offset) override;

    void write8(u8 bus, u8 device, u8 function, u32 offset, u8 value) override;
    void write16(u8 bus, u8 device, u8 function, u32 offset, u16 value) override;
    void write32(u8 bus, u8 device, u8 function, u32 offset, u32 value) override;

private:
    PIIX4Controller() : Controller(0) {}
};


}