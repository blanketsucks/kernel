#pragma once

#include <kernel/common.h>

namespace kernel::ata {

constexpr static u16 PRIMARY_CONTROL_PORT = 0x3F6;
constexpr static u16 SECONDARY_CONTROL_PORT = 0x376;

constexpr static u16 PRIMARY_DATA_PORT = 0x1F0;
constexpr static u16 SECONDARY_DATA_PORT = 0x170;

constexpr static u16 PRIMARY_IRQ = 14;
constexpr static u16 SECONDARY_IRQ = 15;

enum class Drive : u8 {
    Master,
    Slave
};

enum class Channel : u8 {
    Primary,
    Secondary
};

enum Command : u8 {
    Identify       = 0xEC,
    IdentifyPacket = 0xA1,
    CacheFlush     = 0xE7,

    // For 28-bit mode
    Read        = 0x20,
    ReadDMA     = 0xC8,
    Write       = 0x30,
    WriteDMA    = 0xCA,

    // For 48-bit mode
    ReadExt     = 0x24,
    ReadDMAExt  = 0x25,
    WriteExt    = 0x34,
    WriteDMAExt = 0x35,

    Packet = 0xA0,
};

enum SCSICommand : u8 {
    ReadCapacity = 0x25,
    Read10 = 0x28,
    Write10 = 0x2A,
    Read12 = 0xA8,
    Write12 = 0xAA,
};

enum Status : u8 {
    Error = 1 << 0,
    Index = 1 << 1,
    CorrectedData = 1 << 2,
    DataRequest = 1 << 3,
    DriveFault = 1 << 5,
    DataReady = 1 << 6,
    Busy = 1 << 7
};

enum Register : u8 {
    Data = 0x00,
    ErrorReg = 0x01,
    Features = 0x01,
    SectorCount = 0x02,
    LBA0 = 0x03,
    LBA1 = 0x04,
    LBA2 = 0x05,
    DriveReg = 0x06,
    StatusReg = 0x07,
    CommandReg = 0x07,
    Control = 0x0C,
    AltStatus = 0x0C,

    BMStatus = 0x2,
    BMPRDT = 0x4,
    BMRead = 0x8
};

// Some of these fields aren't actually reserved but I named them as such because I don't care about them
struct IdentifyData {
    u16 general_configuration;

    u16 reserved0;

    u16 specific_configuration;

    u16 reserved1[7];

    u8 serial_number[20];

    u16 reserved2[3];

    u8 firmware_version[8];
    u8 model_number[40];

    u16 reserved3[2];

    u16 capabilities[2];

    u16 reserved4[9];

    u32 lba_28_max_addressable_block;

    u16 reserved5[18];

    u16 major_version;
    u16 minor_version;
    u16 command_sets_supported[3];
    u16 command_sets_enabled[3];

    u16 reserved6[12];

    u64 lba_48_max_addressable_block;

    u16 reserved7[2];

    struct {
        u8 log2_logical_sector_size : 4;
        u16 reserved : 8; // I can't have this field be a u8 because gcc keeps crying about the field's offset changing in GCC 4.4 and I can't remove the bit field specifier
                          // because GCC will pad the previous field to be 1 byte even though the struct is declared as packed.
        u8 logical_sector_size_greater_than_256_words : 1;
        u8 multiple_logical_sectors_per_physical_sector : 1;
        u8 one : 1;
        u8 zero : 1;
    } PACKED physical_logical_sector_size;

    u16 reserved8[10];

    u32 words_per_logical_sector;

    u8 reserved[0x200 - 0xEE];
} PACKED;

enum Capabilities : u16 {
    DMASupported = 1 << 8,
};

enum SupportedCommandSets : u16 {
    LBA48Bit = 1 << 10,
};

}