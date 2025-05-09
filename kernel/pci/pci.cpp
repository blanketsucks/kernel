#include <kernel/pci/pci.h>
#include <kernel/pci/address.h>
#include <kernel/pci/device.h>

namespace kernel::pci {

static void enumerate_bus(u8 bus, EnumerationCallback& callback);

static void enumerate_functions(u8 bus, u8 slot, u8 function, EnumerationCallback& callback) {
    Address address = { bus, slot, function };
    
    // Check if the device is a PCI-to-PCI bridge
    if (address.class_id() == 0x6 && address.subclass_id() == 0x4) {
        u8 secondary_bus = address.read<u8>(Address::SecondaryBus);
        enumerate_bus(secondary_bus, callback);
    } else {
        callback({ address });
    }
}

static void enumerate_slot(u8 bus, u8 slot, EnumerationCallback& callback) {
    Address address = { bus, slot, 0 };
    if (address.vendor_id() == 0xFFFF) {
        return;
    }

    for (u8 function = 0; function < 8; function++) {
        Address address = { bus, slot, function };
        if (address.vendor_id() == 0xFFFF) {
            continue;
        }

        enumerate_functions(bus, slot, function, callback);
    }
}

static void enumerate_bus(u8 bus, EnumerationCallback& callback) {
    for (u8 slot = 0; slot < 32; slot++) {
        enumerate_slot(bus, slot, callback);
    }
}

void enumerate(EnumerationCallback&& callback) {
    Address address = { 0, 0, 0 };
    bool has_multiple_functions = address.header_type() & (1 << 7);

    if (!has_multiple_functions) {
        enumerate_bus(0, callback);
        return;
    }

    for (u8 function = 0; function < 8; function++) {
        Address address = { 0, 0, function };
        if (address.vendor_id() == 0xFFFF) {
            continue;
        }

        enumerate_bus(function, callback);
    }
}

}