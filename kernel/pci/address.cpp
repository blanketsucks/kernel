#include <kernel/pci/address.h>
#include <kernel/arch/io.h>

namespace kernel::pci {

union Command {
    struct {
        u16 io_space : 1;
        u16 memory_space : 1;
        u16 bus_master : 1;
        u16 special_cycles : 1;
        u16 memory_write_and_invalidate : 1;
        u16 vga_palette_snoop : 1;
        u16 parity_error_response : 1;
        u16 reserved : 1;
        u16 serr_enable : 1;
        u16 fast_back_to_back : 1;
        u16 interrupt_disable : 1;
    };

    u16 value;

    Command() = default;
    Command(u16 value) : value(value) {}
};

template<> u8 Address::read<u8>(u8 offset) const {
    auto addr = m_value;

    addr.register_offset = offset;
    addr.enable = 1;

    io::write<u32>(CONFIG_ADDRESS, addr.value);
    return io::read<u8>(CONFIG_DATA + (offset & 3));
}

template<> u16 Address::read<u16>(u8 offset) const {
    auto addr = m_value;

    addr.register_offset = offset;
    addr.enable = 1;

    io::write<u32>(CONFIG_ADDRESS, addr.value);
    return io::read<u16>(CONFIG_DATA + (offset & 2));
}

template<> u32 Address::read<u32>(u8 offset) const {
    auto addr = m_value;

    addr.register_offset = offset;
    addr.enable = 1;

    io::write<u32>(CONFIG_ADDRESS, addr.value);
    return io::read<u32>(CONFIG_DATA);
}

template<> void Address::write<u8>(u8 offset, u8 data) {
    auto addr = m_value;

    addr.register_offset = offset;
    addr.enable = 1;

    io::write<u32>(CONFIG_ADDRESS, addr.value);
    io::write<u8>(CONFIG_DATA + (offset & 3), data);
}

template<> void Address::write<u16>(u8 offset, u16 data) {
    auto addr = m_value;

    addr.register_offset = offset;
    addr.enable = 1;

    io::write<u32>(CONFIG_ADDRESS, addr.value);
    io::write<u16>(CONFIG_DATA + (offset & 2), data);
}

template<> void Address::write<u32>(u8 offset, u32 data) {
    auto addr = m_value;

    addr.register_offset = offset;
    addr.enable = 1;

    io::write<u32>(CONFIG_ADDRESS, addr.value);
    io::write<u32>(CONFIG_DATA, data);
}

void Address::set_interrupt_line(bool value) {
    Command command = { read<u16>(CommandReg) };
    command.interrupt_disable = !value;

    write<u16>(CommandReg, command.value);
}

void Address::set_bus_master(bool value) {
    Command command = { read<u16>(CommandReg) };
    command.bus_master = value;

    write<u16>(CommandReg, command.value);
}

void Address::set_io_space(bool value) {
    Command command = { read<u16>(CommandReg) };
    command.io_space = value;

    write<u16>(CommandReg, command.value);
}

u32 Address::bar(u8 index) const {
    return read<u32>(BAR0 + index * 4);
}

BARType Address::bar_type(u8 index) const {
    u32 bar = read<u32>(BAR0 + index * 4);
    if (bar & 1) {
        return BARType::IO;
    }

    switch ((bar >> 1) & 3) {
        case 0:
            return BARType::Memory32;
        case 2:
            return BARType::Memory64;
        default:
            return BARType::Memory16;
    }
}

size_t Address::bar_size(u8 index) {
    u32 bar = read<u32>(BAR0 + index * 4);
    write<u32>(BAR0 + index * 4, 0xFFFFFFFF);

    u32 size = read<u32>(BAR0 + index * 4);
    write<u32>(BAR0 + index * 4, bar);

    size &= 0xFFFFFFF0;
    return ~size + 1;
}

u16 Address::vendor_id() const {
    return read<u16>(VendorID);
}

u16 Address::device_id() const {
    return read<u16>(DeviceID);
}

u16 Address::subclass_id() const {
    return read<u8>(Subclass);
}

u16 Address::class_id() const {
    return read<u8>(ClassCode);
}

u8 Address::prog_if() const {
    return read<u8>(ProgIF);
}

u8 Address::header_type() const {
    return read<u8>(HeaderType);
}

u8 Address::interrupt_line() const {
    return read<u8>(InterruptLine);
}

u8 Address::capabilities_pointer() const {
    return read<u8>(CapabilitiesPointer);
}

template<> u8 Capability::read<u8>(u8 offset) const {
    return m_address.read<u8>(m_cap + offset);
}

template<> u16 Capability::read<u16>(u8 offset) const {
    return m_address.read<u16>(m_cap + offset);
}

template<> u32 Capability::read<u32>(u8 offset) const {
    return m_address.read<u32>(m_cap + offset);
}

template<> void Capability::write<u8>(u8 offset, u8 value) {
    m_address.write<u8>(m_cap + offset, value);
}

template<> void Capability::write<u16>(u8 offset, u16 value) {
    m_address.write<u16>(m_cap + offset, value);
}

template<> void Capability::write<u32>(u8 offset, u32 value) {
    m_address.write<u32>(m_cap + offset, value);
}

}