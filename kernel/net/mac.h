#pragma once

#include <kernel/common.h>

#include <std/array.h>
#include <std/format.h>

namespace kernel::net {

class MACAddress {
public:
    constexpr MACAddress() = default;
    constexpr ~MACAddress() = default;

    constexpr MACAddress(u8 a, u8 b, u8 c, u8 d, u8 e, u8 f) {
        m_data[0] = a;
        m_data[1] = b;
        m_data[2] = c;
        m_data[3] = d;
        m_data[4] = e;
        m_data[5] = f;
    }

    constexpr u8& operator[](size_t index) { return m_data[index]; }
    constexpr u8 const& operator[](size_t index) const { return m_data[index]; }

    constexpr bool operator==(const MACAddress& other) const {
        return m_data[0] == other.m_data[0] &&
               m_data[1] == other.m_data[1] &&
               m_data[2] == other.m_data[2] &&
               m_data[3] == other.m_data[3] &&
               m_data[4] == other.m_data[4] &&
               m_data[5] == other.m_data[5];
    }

    String to_string() const {
        return std::format("{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}", m_data[0], m_data[1], m_data[2], m_data[3], m_data[4], m_data[5]);
    }

private:
    Array<u8, 6> m_data;
};

}

template<>
struct std::Formatter<kernel::net::MACAddress> {
    static void format(FormatBuffer& buffer, const kernel::net::MACAddress& value, const FormatStyle& style) {
        Formatter<String>::format(buffer, value.to_string(), style);
    }
};