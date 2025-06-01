#pragma once

#include <kernel/common.h>
#include <kernel/devices/block_device.h>

#include <std/vector.h>
#include <std/optional.h>

namespace kernel {

struct PartitionEntry {
    size_t index;
    u64 offset;
    u64 size;
};

Vector<PartitionEntry> enumerate_device_partitions(BlockDevice*);

}