#pragma once

#include "std/types.h"
#include <kernel/common.h>
#include <kernel/devices/block_device.h>

#include <std/optional.h>
#include <std/vector.h>
#include <std/memory.h>
#include <std/array.h>

namespace kernel {

struct PartitionEntry {
    u64 offset;
    u64 size;
};

class PartitionTable {
public:
    virtual ~PartitionTable() = default;

    RefPtr<PartitionTable> create(BlockDevice*);

    Optional<PartitionEntry> partition(size_t index) const;
    Vector<PartitionEntry> const& partitions() const { return m_partitions; }

protected:
    PartitionTable(BlockDevice* device) : m_device(device) {}

    BlockDevice* m_device;
    Vector<PartitionEntry> m_partitions;
};

class MBRPartitionTable : public PartitionTable {
public:
    static constexpr u16 SIGNATURE = 0xAA55;

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
        u32 offset;
        u32 sectors;

        bool is_bootable() const {
            return this->attributes & 0x80;
        }
    } PACKED;

    struct MasterBootRecord {
        u8 boostrap[440];
        u32 disk_signature;
        u16 reserved;
        Array<PartitionTableEntry, 4> partitions;
        u16 signature;

        bool is_protective() const {
            return this->partitions[0].type == 0xEE;
        }
    } PACKED;

    bool is_protective() const { return m_is_protective; }

private:
    MBRPartitionTable(BlockDevice* device);

    bool m_is_protective = false;
};

class GPTPartitionTable : public PartitionTable {
public:


private:

};

}