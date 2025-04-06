#pragma once

#include <kernel/common.h>

#include <kernel/net/mac.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/ip/ipv4.h>

#include <std/endian.h>

namespace kernel::net {

struct ARPOperation {
    enum : u16 {
        Request = 1,
        Response = 2
    };
};

struct ARPHardwareType {
    enum : u16 {
        Ethernet = 1
    };
};

struct ARPPacket {
    std::NetworkOrder<u16> hardware_type { ARPHardwareType::Ethernet };
    std::NetworkOrder<u16> protocol_type { EtherType::IPv4 };

    u8 hardware_address_length = sizeof(MACAddress);
    u8 protocol_address_length = sizeof(IPv4Address);

    std::NetworkOrder<u16> operation;

    MACAddress sender_hardware_address;
    IPv4Address sender_protocol_address;

    MACAddress target_hardware_address;
    IPv4Address target_protocol_address;
} PACKED;

}