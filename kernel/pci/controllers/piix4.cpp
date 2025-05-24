#include <kernel/pci/controllers/piix4.h>
#include <kernel/arch/io.h>

namespace kernel::pci {

RefPtr<PIIX4Controller> PIIX4Controller::create() {
    return RefPtr<PIIX4Controller>(new PIIX4Controller());
}

u8 PIIX4Controller::read8(u8 bus, u8 device, u8 function, u32 offset) {
    IOAddress address(bus, device, function);
    address.register_offset = offset;

    io::write<u32>(CONFIG_ADDRESS, address.value);
    return io::read<u8>(CONFIG_DATA + (offset & 0x3));
}

u16 PIIX4Controller::read16(u8 bus, u8 device, u8 function, u32 offset) {
    IOAddress address(bus, device, function);
    address.register_offset = offset;

    io::write<u32>(CONFIG_ADDRESS, address.value);
    return io::read<u16>(CONFIG_DATA + (offset & 0x2));
}

u32 PIIX4Controller::read32(u8 bus, u8 device, u8 function, u32 offset) {
    IOAddress address(bus, device, function);
    address.register_offset = offset;

    io::write<u32>(CONFIG_ADDRESS, address.value);
    return io::read<u32>(CONFIG_DATA);
}

void PIIX4Controller::write8(u8 bus, u8 device, u8 function, u32 offset, u8 value) {
    IOAddress address(bus, device, function);
    address.register_offset = offset;

    io::write<u32>(CONFIG_ADDRESS, address.value);
    io::write<u8>(CONFIG_DATA + (offset & 0x3), value);
}

void PIIX4Controller::write16(u8 bus, u8 device, u8 function, u32 offset, u16 value) {
    IOAddress address(bus, device, function);
    address.register_offset = offset;

    io::write<u32>(CONFIG_ADDRESS, address.value);
    io::write<u16>(CONFIG_DATA + (offset & 0x2), value);
}

void PIIX4Controller::write32(u8 bus, u8 device, u8 function, u32 offset, u32 value) {
    IOAddress address(bus, device, function);
    address.register_offset = offset;

    io::write<u32>(CONFIG_ADDRESS, address.value);
    io::write<u32>(CONFIG_DATA, value);
}

}