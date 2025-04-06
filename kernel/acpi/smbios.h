#pragma once

#include <kernel/common.h>
#include <std/vector.h>

namespace kernel::smbios {

constexpr PhysicalAddress BASE_ADDRESS = 0xF0000;
constexpr size_t MAX_SIZE = 0x10000;

struct EntryPoint32Bit {
    char signature[4];
    u8 checksum;
    u8 length;
    u8 major_version;
    u8 minor_version;
    u16 max_structure_size;
    u8 revision;
    char formatted_area[5];
    char legacy_signature[5];
    u8 checksum2;
    u16 table_length;
    u8 table_address;
    u16 tables_count;
    u8 bcd_revision;
} PACKED;

struct EntryPoint64Bit {
    char signature[5];
    u8 checksum;
    u8 length;
    u8 major_version;
    u8 minor_version;
    u8 document_revision;
    u8 revision;
    u8 reserved;
    u32 table_maximum_size;
    u64 table_address;
} PACKED;

enum class TableType {
    BIOSInformation = 0,
    SystemInformation = 1,
    BaseBoardInformation = 2,
    SystemEnclosureOrChassis = 3,
    ProcessorInformation = 4,
    MemoryControllerInformation = 5,
    MemoryModuleInformation = 6,
    CacheInformation = 7,
    PortConnectorInformation = 8,
    SystemSlotsInformation = 9,
    OnBoardDevicesInformation = 10,
    OEMStrings = 11,
    SystemConfigurationOptions = 12,
    BIOSLanguageInformation = 13,
    GroupAssociations = 14,
    SystemEventLog = 15,
    PhysicalMemoryArray = 16,
    MemoryDevice = 17,
    MemoryErrorInformation32Bit = 18,
    MemoryArrayMappedAddress = 19,
    MemoryDeviceMappedAddress = 20,
    BuiltInPointingDevice = 21,
    PortableBattery = 22,
    SystemReset = 23,
    HardwareSecurity = 24,
    SystemPowerControls = 25,
    VoltageProbe = 26,
    CoolingDevice = 27,
    TemperatureProbe = 28,
    ElectricalCurrentProbe = 29,
    OutOfBandRemoteAccess = 30,
    BootIntegrityServicesBIOSInformation = 31,
    SystemBootInformation = 32,
    MemoryErrorInformation64Bit = 33,
    ManagementDevice = 34,
    ManagementDeviceComponent = 35,
    ManagementDeviceThresholdData = 36,
    MemoryChannel = 37,
    IPMIDeviceInformation = 38,
    SystemPowerSupply = 39,
    AdditionalInformation = 40,
    OnboardDevicesExtendedInformation = 41,
    ManagementControllerHostInterface = 42,
    TPMDevice = 43,
    ProcessorAdditionalInformation = 44,
    Inactive = 126,
    EndOfTable = 127,
};

struct TableHeader {
    u8 type;
    u8 length;
    u16 handle;
} PACKED;

size_t get_table_size(TableHeader* table);

class DMIParser {
public:
    static DMIParser* instance();
    void init();

    bool initialized() const { return m_initialized; }

    const EntryPoint32Bit* entry_point_32_bit() const { return m_32_bit_entry_point; }
    const EntryPoint64Bit* entry_point_64_bit() const { return m_64_bit_entry_point; }

    const Vector<TableHeader*>& table_headers() const { return m_table_headers; }

private:
    void find_entry_points();
    void read_table_headers();

    bool m_initialized = false;

    EntryPoint32Bit* m_32_bit_entry_point = nullptr;
    EntryPoint64Bit* m_64_bit_entry_point = nullptr;

    PhysicalAddress m_table_address;

    size_t m_table_length;
    size_t m_table_count;

    Vector<TableHeader*> m_table_headers;
};

}