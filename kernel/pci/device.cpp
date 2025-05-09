#include "kernel/pci/address.h"
#include <kernel/pci/device.h>

namespace kernel::pci {

u16 Device::vendor_id() const {
    return m_address.read<u16>(Address::VendorID);
}

u16 Device::device_id() const {
    return m_address.read<u16>(Address::DeviceID);
}

DeviceClass Device::class_id() const {
    return static_cast<DeviceClass>(m_address.read<u8>(Address::ClassCode));
}

DeviceSubclass Device::subclass_id() const {
    return static_cast<DeviceSubclass>(m_address.read<u8>(Address::Subclass));
}

StringView Device::class_name() const {
    return get_class_name(class_id());
}

StringView Device::subclass_name() const {
    return get_subclass_name(class_id(), subclass_id());
}

u16 Device::subsystem_vendor_id() const {
    return m_address.read<u16>(Address::SubsystemVendorID);
}

u16 Device::subsystem_id() const {
    return m_address.read<u16>(Address::SubsystemID);
}

void Device::enable_interrupts() {
    m_address.set_interrupt_line(true);
}

void Device::enable_bus_mastering() {
    m_address.set_bus_master(true);
}

Vector<Capability> Device::capabilities() const {
    Vector<Capability> caps;
    
    u8 cap = m_address.capabilities_pointer();
    while (cap) {
        u16 header = m_address.read<u16>(cap);
        u8 id = header & 0xFF;

        caps.append({ cap, id, m_address });
        cap = header >> 8;
    }

    return caps;
}

}