#pragma once

#include <kernel/common.h>
#include <kernel/net/mac.h>

#include <std/endian.h>

namespace kernel::net {

static constexpr size_t MAX_ETHERNET_FRAME_SIZE = 1518;

struct EtherType {
    enum : u16 {
        IPv4 = 0x0800,
        ARP = 0x0806,
        IPv6 = 0x86DD
    };
};

struct EthernetFrame {
    MACAddress destination;
    MACAddress source;
    std::NetworkOrder<u16> type;
    u8 payload[];
} PACKED;

}