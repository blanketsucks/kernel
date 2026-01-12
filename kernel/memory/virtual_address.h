#pragma once

#include <std/types.h>
#include <std/utility.h>

namespace kernel {

class VirtualAddress {
public:
    static constexpr FlatPtr PAGE_MASK = ~(FlatPtr)0xFFF;

    constexpr VirtualAddress() : m_address(0) {}

    constexpr explicit VirtualAddress(FlatPtr address) : m_address(address) {}
    explicit VirtualAddress(const void* address) : m_address(reinterpret_cast<FlatPtr>(address)) {}

    constexpr operator FlatPtr() const { return m_address; }

    [[nodiscard]] constexpr VirtualAddress offset(FlatPtr offset) const {
        return VirtualAddress(m_address + offset);
    }

    [[nodiscard]] u8* to_ptr() const { return reinterpret_cast<u8*>(m_address); }

    [[nodiscard]] constexpr VirtualAddress align_up(size_t alignment) const {
        return VirtualAddress(std::align_up(m_address, alignment));
    }

    [[nodiscard]] constexpr VirtualAddress align_down(size_t alignment) const {
        return VirtualAddress(std::align_down(m_address, alignment));
    }

    constexpr VirtualAddress page_base() const { return VirtualAddress(m_address & PAGE_MASK); }
    constexpr size_t offset_in_page() const { return m_address & ~PAGE_MASK; }
    constexpr bool is_page_aligned() const { return (m_address & ~PAGE_MASK) == 0; }

    constexpr bool operator==(VirtualAddress const& other) const { return m_address == other.m_address; }
    constexpr bool operator!=(VirtualAddress const& other) const { return m_address != other.m_address; }
    constexpr bool operator<(VirtualAddress const& other) const { return m_address < other.m_address; }
    constexpr bool operator<=(VirtualAddress const& other) const { return m_address <= other.m_address; }
    constexpr bool operator>(VirtualAddress const& other) const { return m_address > other.m_address; }
    constexpr bool operator>=(VirtualAddress const& other) const { return m_address >= other.m_address; }

    constexpr VirtualAddress operator-(VirtualAddress const& other) const { return VirtualAddress(m_address - other.m_address); }
    constexpr VirtualAddress operator-(FlatPtr offset) const { return VirtualAddress(m_address - offset); }

private:
    FlatPtr m_address;
};

static constexpr VirtualAddress MAX_VIRTUAL_ADDRESS = VirtualAddress(~(FlatPtr)0);

}