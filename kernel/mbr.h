#pragma once

#include <kernel/common.h>
#include <std/array.h>

namespace kernel {

constexpr u16 MBR_SIGNATURE = 0xAA55;

struct CHSAddress {
    u8 head;
    u8 sector    : 6;
    u16 cylinder : 10;
} PACKED;

struct PartitionTableEntry {
    u8 attributes;
    CHSAddress start;
    u8 type;
    CHSAddress end;
    u32 lba_start;
    u32 sectors;

    bool is_bootable() const { return attributes & 0x80; }
    bool is_protective_mbr() const { return type == 0xEE; }
} PACKED;

static_assert(sizeof(PartitionTableEntry) == 16);

struct MasterBootRecord {
    u8 boostrap[440];
    u32 disk_signature;
    u16 reserved;
    Array<PartitionTableEntry, 4> partitions;
    u16 signature;
} PACKED;

static_assert(sizeof(MasterBootRecord) == 512);

}