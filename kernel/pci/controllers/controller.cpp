#include <kernel/pci/controllers/controller.h>
#include <kernel/pci/address.h>

namespace kernel::pci {

void Controller::enumerate(Function<void(const Device&)>&& callback) {
    u8 header_type = read8(0, 0, 0, Address::HeaderType);
    bool has_multiple_functions = header_type & (1 << 7);

    if (!has_multiple_functions) {
        this->enumerate_bus(0, callback);
        return;
    }

    for (u8 function = 0; function < 8; function++) {
        u16 vendor_id = read16(0, 0, function, Address::VendorID);
        if (vendor_id == 0xFFFF) {
            continue;
        }

        this->enumerate_bus(function, callback);
    }
}

void Controller::enumerate_bus(u8 bus, Function<void(const Device&)>& callback) {
    for (u8 slot = 0; slot < 32; slot++) {
        this->enumerate_slot(bus, slot, callback);
    }
}

void Controller::enumerate_slot(u8 bus, u8 slot, Function<void(const Device&)>& callback) {
    u16 vendor_id = read16(bus, slot, 0, Address::VendorID);
    if (vendor_id == 0xFFFF) {
        return;
    }

    for (u8 function = 0; function < 8; function++) {
        u16 vendor_id = read16(bus, slot, function, Address::VendorID);
        if (vendor_id == 0xFFFF) {
            continue;
        }

        this->enumerate_functions(bus, slot, function, callback);
    }
}

void Controller::enumerate_functions(u8 bus, u8 slot, u8 function, Function<void(const Device&)>& callback) {
    u16 class_id = read16(bus, slot, function, Address::ClassCode);
    u16 subclass_id = read16(bus, slot, function, Address::Subclass);

    // Check if the device is a PCI-to-PCI bridge
    if (class_id == 0x6 && subclass_id == 0x4) {
        u8 secondary_bus = read8(bus, slot, function, Address::SecondaryBus);
        this->enumerate_bus(secondary_bus, callback);
    } else {
        Address address = { bus, slot, function };
        callback({ address });
    }
}


}