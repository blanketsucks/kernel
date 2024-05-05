#pragma once

#include <kernel/common.h>

namespace kernel::apic {

enum class APICRegisters : u32 {
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
    InService = 0x100,        // From 0x100 to 0x170
    TriggerMode = 0x180,      // From 0x180 to 0x1F0
    InterruptRequest = 0x200, // From 0x200 to 0x270
    ErrorStatus = 0x280,
    LVT_CMCI = 0x2F0,
    InterruptCommand = 0x300, // From 0x300 to 0x310
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

constexpr u32 IA32_APIC_BASE = 0x1B;
constexpr u32 IA32_APIC_BASE_BSP = 0x100;
constexpr u32 IA32_APIC_BASE_ENABLE = 0x800;

constexpr u8 SPURIOUS_INTERRUPT_VECTOR = 0xFF;

void write_reg(APICRegisters reg, u32 value);
void write_reg(u32 reg, u32 value);

u32 read_reg(APICRegisters reg);
u32 read_reg(u32 reg);

void init();

}