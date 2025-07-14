#include <kernel/devices/storage/partitions.h>

#include <std/array.h>
#include <std/format.h>

namespace kernel {

struct PartitionTableEntry {
    u8 attributes;
    u32 start : 24;
    u8 type;
    u32 end : 24;
    u32 offset;
    u32 sectors;
} PACKED;

struct MasterBootRecord {
    static constexpr u16 SIGNATURE = 0xAA55;

    u8 boostrap[440];
    u32 disk_signature;
    u16 reserved;
    PartitionTableEntry partitions[4];
    u16 signature;
} PACKED;

struct GPTHeader {
    u8 signature[8];
    u32 revision;
    u32 header_size;
    u32 header_crc;
    u32 reserved;
    u64 current_lba;
    u64 backup_lba;
    u64 first_usable_lba;
    u64 last_usable_lba;
    u8 disk_guid[16];
    u64 partition_table_lba;
    u32 partition_count;
    u32 partition_entry_size;
    u32 partition_table_crc;
} PACKED;

struct GPTEntry {
    u8 partition_type_guid[16];
    u8 unique_partition_guid[16];
    u64 first_lba;
    u64 last_lba;
    u64 attributes;
    u16 name[36];
    
    bool is_valid() const {
        return std::any(partition_type_guid, [](u8 byte) { return byte != 0; });
    }
} PACKED;

Vector<PartitionEntry> parse_gpt_partitions(BlockDevice*);

Vector<PartitionEntry> enumerate_device_partitions(BlockDevice* device) {
    MasterBootRecord mbr = {};
    auto result = device->read_blocks(reinterpret_cast<u8*>(&mbr), 1, 0);

    if (result.is_err()) {
        // TODO: Report the error
        return {};
    }

    if (mbr.signature != MasterBootRecord::SIGNATURE) {
        return {};
    }
    
    bool is_protective = mbr.partitions[0].type == 0xEE;
    if (is_protective) {
        return parse_gpt_partitions(device);
    }
    
    Vector<PartitionEntry> partitions;
    size_t index = 0;

    for (auto& partition : mbr.partitions) {
        if (!partition.offset) {
            continue;
        }

        partitions.append({ index++, partition.offset, partition.sectors });
    }

    return partitions;
}

Vector<PartitionEntry> parse_gpt_partitions(BlockDevice* device) {
    u8 block[device->block_size()];
    auto result = device->read_blocks(block, 1, 1);

    if (result.is_err()) {
        // TODO: Report the error
        return {};
    }

    GPTHeader* gpt = reinterpret_cast<GPTHeader*>(block);
    if (memcmp(gpt->signature, "EFI PART", 8) != 0) {
        return {};
    }
    
    Vector<PartitionEntry> partitions;

    // FIXME: Do not read every single entry at once but rather read them in chunks.
    Vector<GPTEntry> entries;
    entries.resize(gpt->partition_count);

    size_t blocks = (gpt->partition_count * gpt->partition_entry_size) / device->block_size();
    result = device->read_blocks(reinterpret_cast<u8*>(entries.data()), blocks, gpt->partition_table_lba);

    if (result.is_err()) {
        // TODO: Report the error
        return {};
    }

    for (size_t i = 0; i < gpt->partition_count; i++) {
        auto& entry = entries[i];
        if (!entry.is_valid()) {
            continue;
        }

        partitions.append({ i, entry.first_lba, entry.last_lba - entry.first_lba + 1 });
    }

    return partitions;
}

}