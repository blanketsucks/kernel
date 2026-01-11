#pragma once

#include <kernel/common.h>
#include <kernel/acpi/acpi.h>

namespace kernel {

struct TimerConfiguration {
    u64 configuration_and_capabilities;
    u64 comparator_value;
    u64 interrupt_route;
    u64 reserved;
};

struct HPETRegisters {
    u64 general_capabilities_and_id;
    u64 reserved;
    u64 general_configuration;
    u64 reserved1;
    u64 interrupt_status;
    u8 reserved2[0xEF - 0x27];
    u64 main_counter_value;
    u64 reserved3;
    TimerConfiguration timers[32];
};

union GeneralCapabilitiesAndID {
    struct {
        u8 rev_id;
        u8 num_timers : 5;
        u8 count_size_cap : 1;
        u8 reserved : 1;
        u8 legacy_replacement : 1;
        u16 pci_vendor_id;
        u32 counter_clock_period;
    };
    
    u64 value;

    GeneralCapabilitiesAndID(u64 value = 0) : value(value) {}
};

union TimerConfigurationAndCapabilities {
    struct {
        u8 : 1;
        u8 timer_type : 1;
        u8 interrupt_enable : 1;
        u8 periodic_enable : 1;
        u8 periodic_capable : 1;
        u8 size_capable : 1;
        u8 : 2;
        u8 mode : 1; // If 64-bit and 1, then timer will work in 32-bit mode
        u8 interrupt_route : 5;
        u8 fsb_enable : 1;
        u8 fsb_capable : 1;
        u32 : 16;
        u32 interrupt_route_capability;
    };
    
    u64 value;

    TimerConfigurationAndCapabilities(u64 value = 0) : value(value) {}
};

struct HPETTable {
    static constexpr const char* SIGNATURE = "HPET";

    acpi::SDTHeader header;
    u8 hardware_rev_id;
    u8 comparator_count : 5;
    u8 count_size_cap : 1;
    u8 reserved : 1;
    u8 legacy_replacement : 1;
    u16 pci_vendor_id;
    acpi::GenericAddressStructure address;
    u8 hpet_number;
    u16 minimum_tick;
    u8 page_protection;
};

}