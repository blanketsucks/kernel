#pragma once

#include <kernel/common.h>

namespace kernel::net {

struct UDPPacket {
    u16 source_port;
    u16 destination_port;
    u16 length;
    u16 checksum;
    u8 data[];
} PACKED;

}