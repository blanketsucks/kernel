#pragma once

#include <kernel/common.h>

namespace kernel::ahci {

enum class PortSignature : u32 {
    ATA = 0x00000101,
    ATAPI = 0xEB140101,
    SEMB = 0xC33C0101,
    PM = 0x96690101
};

// The upper two bytes represent the major version number, and the lower two bytes represent the minor version number.
enum AHCIVersion : u32 {
    AHCI_0_95 = 0x00090500,
    AHCI_1_0  = 0x00010000,
    AHCI_1_1  = 0x00010100,
    AHCI_1_2  = 0x00010200,
    AHCI_1_3  = 0x00010300,
    AHCI_1_31 = 0x00010301,
};

enum PxCMD : u32 {
    ST = 1 << 0,  // Start
    SUD = 1 << 1, // Spin Up Device
    FRE = 1 << 4, // FIS Receive Enable
    FR = 1 << 14, // FIS Receive Running
    CR = 1 << 15, // Command List Running
    CPD = 1 << 20, // Cold Presence Detected
};

enum PxIS : u32 {
    DHRS = 1 << 0, // Device to Host Register FIS Interrupt
    PSS = 1 << 1,  // PIO Setup FIS Interrupt
    DSS = 1 << 2,  // DMA Setup FIS Interrupt
    DSBS = 1 << 3, // Set Device Bits Interrupt
    UFS = 1 << 4,  // Unknown FIS Interrupt
    DPS = 1 << 5,  // Descriptor Processed
    PCS = 1 << 6,  // Port Connect Change Status
    DMPS = 1 << 7, // Device Mechanical Presence Status
    PRCS = 1 << 22, // PhyRdy Change Status
    IMPS = 1 << 23, // Incorrect Port Multiplier Status
    OFS = 1 << 24,  // Overflow Status
    INFS = 1 << 26, // Interface Non-fatal Error Status
    IES = 1 << 27,  // Interface Fatal Error Status
    HBDS = 1 << 28, // Host Bus Data Error Status
    HBFS = 1 << 29, // Host Bus Fatal Error Status
    TFES = 1 << 30, // Task File Error Status
    CPDS = 1u << 31, // Cold Port Detect Status
};

enum Capabilities : u32 {
    S64A = 1u << 31, // Supports 64-bit Addressing
};

enum ExtendedCapabilities : u32 {
    BOH = 1 << 0, // BIOS/OS Handoff
};

enum GlobalHostControl : u32 {
    HR = 1 << 0,  // HBA Reset
    IE = 1 << 1,  // Interrupt Enable
    AE = 1u << 31, // AHCI Enable
};

struct HBAPort {
    u32 command_list;
    u32 command_list_upper;
    u32 fis_base;
    u32 fis_base_upper;
    u32 interrupt_status;
    u32 interrupt_enable;
    u32 cmd;
    
    u32 reserved1;

    u32 task_file_data;
    u32 signature;
    u32 sata_status;
    u32 sata_control;
    u32 sata_error;
    u32 sata_active;
    u32 command_issue;
    u32 sata_notification;
    u32 fis_based_switch_control;
    u32 reserved[11];

    u32 vendor[4];
};

struct HandoffControlStatus {
    u8 bos : 1;  // BIOS owned semaphore
    u8 oos : 1;  // OS owned semaphore
    u8 sooe : 1; // SMI on OS owned semaphore enable
    u8 ooc : 1;  // OS ownership change
    u8 bb : 1;   // BIOS busy
    u32 reserved : 27;
};

struct HBAMemory {
    u32 capabilities;
    u32 global_host_control;
    u32 interrupt_status;
    u32 ports_implemented;
    u32 version;
    u32 command_completion_coalescing_control;
    u32 command_completion_coalescing_ports;
    u32 enclosure_management_location;
    u32 enclosure_management_control;
    u32 host_capabilities_extended;
    HandoffControlStatus bios_os_handoff_control_status;

    u8 reserved[0xA0 - 0x2C];
    u8 vendor[0x100 - 0xA0];

    HBAPort ports[32];
};

struct CommandHeader {
    u8 fis_length : 5;
    u8 atapi : 1;
    u8 write : 1;
    u8 prefetchable : 1;
    u8 reset : 1;
    u8 bist : 1;
    u8 clear_busy : 1;
    u8 : 1;
    u8 port_multiplier : 4;
    u16 prdt_length;
    u32 prdb_count;
    u32 command_table_base;
    u32 command_table_base_upper;

    u32 reserved[4];
};

struct PhysicalRegionDescriptor {
    u32 data_base;
    u32 data_base_upper;
    u32 reserved;
    u32 byte_count : 22;
    u16 : 9;
    u8 interrupt_on_completion : 1;
};

struct CommandTable {
    u8 command_fis[64];
    u8 atapi_command[16];
    u8 reserved[48];
    PhysicalRegionDescriptor prdt[];
};

struct FISRegisterH2D {
    enum Type {
        H2D = 0x27,
    };

    u8 type;
    u8 pm_port : 4;
    u8 : 3;
    u8 c : 1;
    u8 command;
    u8 feature_low;

    u8 lba0;
    u8 lba1;
    u8 lba2;
    u8 device;

    u8 lba3;
    u8 lba4;
    u8 lba5;
    u8 feature_high;

    u8 count_low;
    u8 count_high;
    u8 icc;
    u8 control;

    u8 reserved[4];
};
    

}