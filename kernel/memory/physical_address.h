#pragma once

#include <std/types.h>
#include <std/utility.h>

namespace kernel {

class PhysicalAddress {
public:
    static constexpr FlatPtr PAGE_MASK = ~(FlatPtr)0xFFF;

    constexpr PhysicalAddress() : m_address(0) {}

    constexpr explicit PhysicalAddress(FlatPtr address) : m_address(address) {}
    explicit PhysicalAddress(const void* address) : m_address(reinterpret_cast<FlatPtr>(address)) {}

    constexpr operator FlatPtr() const { return m_address; }
    constexpr FlatPtr value() const { return m_address; }

    [[nodiscard]] constexpr PhysicalAddress offset(FlatPtr offset) const {
        return PhysicalAddress(m_address + offset);
    }

    [[nodiscard]] u8* to_ptr() const { return reinterpret_cast<u8*>(m_address); }

    [[nodiscard]] constexpr PhysicalAddress align_up(size_t alignment) const {
        return PhysicalAddress(std::align_up(m_address, alignment));
    }

    [[nodiscard]] constexpr PhysicalAddress align_down(size_t alignment) const {
        return PhysicalAddress(std::align_down(m_address, alignment));
    }

    constexpr PhysicalAddress page_base() const { return PhysicalAddress(m_address & PAGE_MASK); }
    constexpr size_t offset_in_page() const { return m_address & ~PAGE_MASK; }
    constexpr bool is_page_aligned() const { return (m_address & ~PAGE_MASK) == 0; }

    constexpr bool operator==(PhysicalAddress const& other) const { return m_address == other.m_address; }
    constexpr bool operator!=(PhysicalAddress const& other) const { return m_address != other.m_address; }
    constexpr bool operator<(PhysicalAddress const& other) const { return m_address < other.m_address; }
    constexpr bool operator<=(PhysicalAddress const& other) const { return m_address <= other.m_address; }
    constexpr bool operator>(PhysicalAddress const& other) const { return m_address > other.m_address; }
    constexpr bool operator>=(PhysicalAddress const& other) const { return m_address >= other.m_address; }

    constexpr PhysicalAddress operator-(PhysicalAddress const& other) const { return PhysicalAddress(m_address - other.m_address); }
    constexpr PhysicalAddress operator-(FlatPtr offset) const { return PhysicalAddress(m_address - offset); }

private:
    FlatPtr m_address;
};

}