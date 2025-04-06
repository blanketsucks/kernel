#pragma once

#include <kernel/common.h>

#include <std/format.h>
#include <std/endian.h>

namespace kernel::net {

class IPv4Address {
public:
    IPv4Address() = default;

    IPv4Address(u8 a, u8 b, u8 c, u8 d) {
        m_value.m_bytes[0] = a;
        m_value.m_bytes[1] = b;
        m_value.m_bytes[2] = c;
        m_value.m_bytes[3] = d;
    }

    IPv4Address(u32 address) {
        m_value.m_address = address;
    }
    
    static IPv4Address any() {
        return IPv4Address(0);
    }

    u8 operator[](size_t index) const {
        return m_value.m_bytes[index];
    }

    bool operator==(const IPv4Address& other) const {
        return m_value.m_address == other.m_value.m_address;
    }

    bool operator!=(const IPv4Address& other) const {
        return m_value.m_address != other.m_value.m_address;
    }

    u32 value() const {
        return m_value.m_address;
    }

    bool is_zero() const {
        return m_value.m_address == 0;
    }

    bool is_broadcast() const {
        return m_value.m_address == 0xFFFFFFFF;
    }

    String to_string() const {
        return std::format(
            "{}.{}.{}.{}",
            m_value.m_bytes[0],
            m_value.m_bytes[1],
            m_value.m_bytes[2],
            m_value.m_bytes[3]
        );
    }
private:
    union {
        u32 m_address = 0;
        u8 m_bytes[4];
    } PACKED m_value;
};

struct IPv4Packet {
    u8 version : 4;
    u8 ihl : 4;
    u8 dscp : 6;
    u8 ecn : 2;
    std::NetworkOrder<u16> length;
    std::NetworkOrder<u16> identification;
    std::NetworkOrder<u16> flags_and_fragment_offset;
    u8 ttl;
    std::NetworkOrder<u8> protocol;
    std::NetworkOrder<u16> checksum;
    IPv4Address source;
    IPv4Address destination;
    u8 payload[0];
} PACKED;

}

template<>
struct std::Formatter<kernel::net::IPv4Address> {
    static void format(FormatBuffer& buffer, const kernel::net::IPv4Address& value, const FormatStyle& style) {
        Formatter<String>::format(buffer, value.to_string(), style);
    }
};