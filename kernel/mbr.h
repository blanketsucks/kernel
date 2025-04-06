#pragma once

#include <kernel/common.h>

#include <std/utility.h>
#include <std/array.h>
#include <std/string.h>

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
    u32 offset;
    u32 sectors;

    bool is_bootable() const {
        return this->attributes & 0x80;
    }
} PACKED;

static_assert(sizeof(PartitionTableEntry) == 16);

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

static_assert(sizeof(MasterBootRecord) == 512);

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
    Array<u8, 16> partition_type_guid;
    Array<u8, 16> unique_partition_guid;
    u64 first_lba;
    u64 last_lba;
    u64 attributes;
    Array<u16, 36> partition_name;

    String name() const {
        // FIXME: name is UNICODE16-LE encoded
        String name;
        name.resize(partition_name.size());

        for (size_t i = 0; i < partition_name.size(); i++) {
            u8 c = partition_name[i] & 0xFF;
            if (c == 0) {
                break;
            }

            name.append(c);
        }

        return name;
    }

    bool is_efi_system_partition() const {
        static constexpr u8 EFI_SYSTEM_PARTITION_GUID[] = {
            0x28, 0x73, 0x2a, 0xc1, 0x1f, 0xf8, 0xd2, 0x11, 
            0xba, 0x4b, 0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b,
        };

        return std::memcmp(partition_type_guid.data(), EFI_SYSTEM_PARTITION_GUID, 16) == 0;
    }
    
    bool is_valid() const {
        return std::any(partition_type_guid, [](u8 byte) { return byte != 0; });
    }

} PACKED;

}