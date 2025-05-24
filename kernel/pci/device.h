#pragma once

#include <kernel/pci/address.h>
#include <kernel/pci/defs.h>

#include <std/vector.h>

namespace kernel::pci {

class Device {
public:
    Device() = default;
    Device(Address address) : m_address(address) {}

    Address address() const { return m_address; }

    u16 vendor_id() const;
    u16 device_id() const;

    DeviceClass class_id() const;
    DeviceSubclass subclass_id() const;

    StringView class_name() const;
    StringView subclass_name() const;

    u16 subsystem_vendor_id() const;
    u16 subsystem_id() const;

    u8 interrupt_line() const { return m_address.interrupt_line(); }

    void enable_interrupts();
    void enable_bus_mastering();

    u32 bar0() const { return m_address.bar(0); }
    u32 bar1() const { return m_address.bar(1); }
    u32 bar2() const { return m_address.bar(2); }
    u32 bar3() const { return m_address.bar(3); }
    u32 bar4() const { return m_address.bar(4); }
    u32 bar5() const { return m_address.bar(5); }

    u32 bar(u8 index) const { return m_address.bar(index); }

    BARType bar_type(u8 index) const { return m_address.bar_type(index); }
    size_t bar_size(u8 index) { return m_address.bar_size(index); }

    Vector<Capability> capabilities() const;

    bool is_ide_controller() const {
        return class_id() == DeviceClass::MassStorageController && subclass_id() == DeviceSubclass::IDEController;
    }

    bool is_sata_controller() const {
        return class_id() == DeviceClass::MassStorageController && subclass_id() == DeviceSubclass::SATAController;
    }

    bool is_audio_device() const {
        return class_id() == DeviceClass::MultimediaController && subclass_id() == DeviceSubclass::MultimediaAudioController;
    }

    bool is_usb_controller() const {
        return class_id() == DeviceClass::SerialBusController && subclass_id() == DeviceSubclass::USBController;
    }

    bool is_display_controller() const {
        return class_id() == DeviceClass::DisplayController;
    }

    bool is_virtio_device() const {
        return vendor_id() == 0x1AF4;
    }

private:
    Address m_address;
};

}