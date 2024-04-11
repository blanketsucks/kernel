#pragma once

#include <kernel/common.h>
#include <std/vector.h>

namespace kernel::acpi {

constexpr u32 RSDP_START = 0x000E0000;
constexpr u32 RSDP_END = 0x000FFFFF;

struct RSDP {
    char signature[8];
    u8 checksum;
    char oem_id[6];
    u8 revision;
    u32 rsdt_address;
} PACKED;

struct XSDP {
    char signature[8];
    u8 checksum;
    char oem_id[6];
    u8 revision;
    u32 rsdt_address; // deprecated

    u32 length;
    u64 xsdt_address;
    u8 extended_checksum;
    u8 reserved[3];
} PACKED;

struct SDTHeader {
    char signature[4];
    u32 length;
    u8 revision;
    u8 checksum;
    char oem_id[6];
    char oem_table_id[8];
    u32 oem_revision;
    u32 creator_id;
    u32 creator_revision;
} PACKED;

struct RSDT {
    SDTHeader header;
    u32 tables[];
} PACKED;

struct MADT {
    SDTHeader header;

    u32 local_apic_address;
    u32 flags;
} PACKED;

enum class MADTEntryType : u8 {
    ProcessorLocalAPIC = 0,
    IOAPIC = 1,
    InterruptSourceOverride = 2,
    NMISource = 3,
    LocalAPICAddressOverride = 5,
    IOAPICAddressOverride = 6,
    LocalAPICX2 = 9
};

struct MADTHeader {
    MADTEntryType type;
    u8 length;
} PACKED;

struct ProcessorLocalAPIC {
    MADTHeader header;

    u8 acpi_processor_id;
    u8 apic_id;
    u32 flags;
} PACKED;

struct IOAPIC {
    MADTHeader header;

    u8 io_apic_id;
    u8 reserved;
    u32 io_apic_address;
    u32 global_system_interrupt_base;
} PACKED;

struct GenericAddressStructure {
    u8 address_space;
    u8 bit_width;
    u8 bit_offset;
    u8 access_size;
    u64 address;
} PACKED;  

struct FADT {
    SDTHeader header;

    u32 firmware_control;
    u32 dsdt;

    u8 reserved;

    u8 preferred_pm_profile;
    u16 sci_interrupt;
    u32 smi_command_port;
    u8 acpi_enable;
    u8 acpi_disable;
    u8 s4bios_request;
    u8 pstate_control;
    u32 pm1a_event_block;
    u32 pm1b_event_block;
    u32 pm1a_control_block;
    u32 pm1b_control_block;
    u32 pm2_control_block;
    u32 pm_timer_block;
    u32 gpe0_block;
    u32 gpe1_block;
    u8 pm1_event_length;
    u8 pm1_control_length;
    u8 pm2_control_length;
    u8 pm_timer_length;
    u8 gpe0_length;
    u8 gpe1_length;
    u8 gpe1_base;
    u8 cstate_control;
    u16 worst_c2_latency;
    u16 worst_c3_latency;
    u16 flush_size;
    u16 flush_stride;
    u8 duty_offset;
    u8 duty_width;
    u8 day_alarm;
    u8 month_alarm;
    u8 century;
    u16 boot_architecture_flags;
    u8 reserved2;
    u32 flags;
    GenericAddressStructure reset_reg;
    u8 reset_value;
    u8 reserved3[3];
    u64 x_firmware_control;
    u64 x_dsdt;

    GenericAddressStructure x_pm1a_event_block;
    GenericAddressStructure x_pm1b_event_block;
    GenericAddressStructure x_pm1a_control_block;
    GenericAddressStructure x_pm1b_control_block;
    GenericAddressStructure x_pm2_control_block;
    GenericAddressStructure x_pm_timer_block;
    GenericAddressStructure x_gpe0_block;
    GenericAddressStructure x_gpe1_block;
} PACKED;

class Parser {
public:
    static Parser* instance();
    void init();

    template<typename T>
    T* find_table(const char* signature) {
        return reinterpret_cast<T*>(this->find_table(signature));
    }

    SDTHeader* find_table(const char* signature);
private:
    bool find_rsdt();
    void parse_acpi_tables();

    SDTHeader* map_acpi_table(u32 address);

    RSDP* m_rsdp = nullptr;
    RSDT* m_rsdt = nullptr;

    SDTHeader* m_dsdt = nullptr;
    

    Vector<SDTHeader*> m_tables;
};

}