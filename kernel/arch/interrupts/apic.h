#pragma once

#include <kernel/common.h>
#include <kernel/arch/processor.h>

#include <std/vector.h>

namespace kernel {

union ICR {
    enum DeliveryMode : u8 {
        Normal = 0,
        LowestPriority = 1,
        SMI = 2,
        NMI = 4,
        INIT = 5,
        SIPI = 6
    };

    enum DestinationMode : u8 {
        Physical = 0,
        Logical = 1
    };

    enum Level : u8 {
        DeAssert = 0,
        Assert = 1
    };

    enum TriggerMode {
        Edge = 0,
        Level = 1
    };

    enum DestinationType {
        Ignore = 0,
        Self = 1,
        All = 2,
        AllButSelf = 3
    };

    struct {
        u8 vector;
        u8 delivery_mode : 3;
        u8 destination_mode : 1;
        u8 delivery_status : 1;
        u8 : 1;
        u8 level : 1;
        u8 trigger_mode : 1;
        u8 : 2;
        u8 destination_type : 2;
        u64 : 36;
        u8 target : 4;
        u8 : 4;
    } PACKED;

    struct {
        u32 low;
        u32 high;
    };
};

class APIC {
public:
    static constexpr u32 IA32_APIC_BASE = 0x1B;
    static constexpr u32 IA32_APIC_BASE_BSP = 0x100;
    static constexpr u32 IA32_APIC_BASE_ENABLE = 0x800;

    static constexpr u8 SPURIOUS_INTERRUPT_VECTOR = 0xFF;

    enum APICRegisters : u32 {
        ID = 0x20,
        Version = 0x30,
        TaskPriority = 0x80,
        ArbitrationPriority = 0x90,
        ProcessorPriority = 0xA0,
        EOI = 0xB0,
        RemoteRead = 0xC0,
        LogicalDestination = 0xD0,
        DestinationFormat = 0xE0,
        SpuriousInterruptVector = 0xF0,
        ErrorStatus = 0x280,
        LVT_CMCI = 0x2F0,
        InterruptCommandRegister = 0x300, // From 0x300 to 0x310
        LVTTimer = 0x320,
        LVTThermalSensor = 0x330,
        LVTPerformanceMonitor = 0x340,
        LVTLint0 = 0x350,
        LVTLint1 = 0x360,
        LVTError = 0x370,
        InitialCount = 0x380,
        CurrentCount = 0x390,
        DivideConfiguration = 0x3E0
    };

    static APIC& instance();
    static void initialize();

    static void boot_all();
    static Processor* get_processor_for(int ap);

    void enable(int cpu);
    void eoi();

private:
    void init();

    void setup_ap_trampoline();
    void do_boot_all();

    void write_reg(u32 reg, u32 value);
    u32  read_reg(u32 reg);

    void write_icr(
        u8 vector,
        u8 destination,
        u8 delivery_mode,
        u8 destination_mode,
        u8 level,
        u8 trigger_mode,
        u8 destination_type
    );

    VirtualAddress m_apic_base;

    u32 m_processors;
    PhysicalAddress m_trampoline;

    Vector<Processor*> m_ap_processors;
    Vector<VirtualAddress> m_ap_stacks;
};

}