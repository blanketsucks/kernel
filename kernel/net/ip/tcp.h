#pragma once

#include <kernel/common.h>
#include <std/endian.h>

namespace kernel::net {

struct TCPPacket {
    std::NetworkOrder<u16> source_port;
    std::NetworkOrder<u16> destination_port;
    std::NetworkOrder<u32> sequence_number;
    std::NetworkOrder<u32> ack_number;
    std::NetworkOrder<u8> data_offset;
    std::NetworkOrder<u8> flags;
    // std::NetworkOrder<u16> flags_and_data_offset;
    std::NetworkOrder<u16> window_size;
    std::NetworkOrder<u16> checksum;
    std::NetworkOrder<u16> urgent_pointer;
    u8 data[];
} PACKED;

}