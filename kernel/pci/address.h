#pragma once

#include <kernel/common.h>

namespace kernel::pci {

enum class BARType {
    IO = 1,
    Memory16,
    Memory32,
    Memory64,
};

class Address {
public:
    static constexpr u16 CONFIG_ADDRESS = 0xCF8;
    static constexpr u16 CONFIG_DATA = 0xCFC;

    enum Register {
        VendorID = 0x00,
        DeviceID = 0x02,
        CommandReg = 0x04,
        Status = 0x06,
        RevisionID = 0x08,
        ProgIF = 0x09,
        Subclass = 0x0A,
        ClassCode = 0x0B,

        HeaderType = 0x0E,

        BAR0 = 0x10,
        BAR1 = 0x14,
        BAR2 = 0x18,
        BAR3 = 0x1C,
        BAR4 = 0x20,
        BAR5 = 0x24,

        SubsystemVendorID = 0x2C,
        SubsystemID = 0x2E,
        ROMBaseAddress = 0x30,
        CapabilitiesPointer = 0x34,
        InterruptLine = 0x3C,
        InterruptPin = 0x3D,

        SecondaryBus = 0x19,
    };

    Address() : m_value({ .value = 0 }) {}
    Address(u8 bus, u8 device, u8 function) : m_value({ .value = 0 }) {
        m_value.bus = bus;
        m_value.device = device;
        m_value.function = function;
    }

    u32 value() const { return m_value.value; }

    u8 bus() const { return m_value.bus; }
    u8 device() const { return m_value.device; }
    u8 function() const { return m_value.function; }

    template<typename T> T read(u8 offset) const = delete;
    template<typename T> void write(u8 offset, T value) = delete;

    void set_interrupt_line(bool value);
    void set_bus_master(bool value);
    void set_io_space(bool value);

    u16 vendor_id() const;
    u16 device_id() const;
    u16 subclass_id() const;
    u16 class_id() const;
    u8 prog_if() const;

    u8 capabilities_pointer() const;

    u8 header_type() const;

    u32 bar(u8 index) const;
    BARType bar_type(u8 index) const;
    size_t bar_size(u8 index);

    u8 interrupt_line() const;
    
private:
    union {
        struct {
            u32 register_offset : 8;
            u32 function : 3;
            u32 device : 5;
            u32 bus : 8;
            u32 reserved : 7;
            u32 enable : 1;
        };

        u32 value;
    } m_value;
};

class Capability {
public:
    Capability(u8 cap, u8 id, Address address) : m_cap(cap), m_id(id), m_address(address) {}

    u8 cap() const { return m_cap; }
    u8 id() const { return m_id; }
    Address address() const { return m_address; }

    template<typename T> T read(u8 offset) const = delete;
    template<typename T> void write(u8 offset, T value) = delete;

private:
    u8 m_cap;
    u8 m_id;
    Address m_address;
};

template<> u8 Address::read<u8>(u8 offset) const;
template<> u16 Address::read<u16>(u8 offset) const;
template<> u32 Address::read<u32>(u8 offset) const;

template<> void Address::write<u8>(u8 offset, u8 value);
template<> void Address::write<u16>(u8 offset, u16 value);
template<> void Address::write<u32>(u8 offset, u32 value);

template<> u8 Capability::read<u8>(u8 offset) const;
template<> u16 Capability::read<u16>(u8 offset) const;
template<> u32 Capability::read<u32>(u8 offset) const;

template<> void Capability::write<u8>(u8 offset, u8 value);
template<> void Capability::write<u16>(u8 offset, u16 value);
template<> void Capability::write<u32>(u8 offset, u32 value);

}