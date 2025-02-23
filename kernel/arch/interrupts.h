#pragma once

#include <kernel/common.h>

namespace kernel::arch {

enum Interrupt : u32 {
    DevideByZero = 0,
    Debug = 1,
    NonMaskableInterrupt = 2,
    Breakpoint = 3,
    Overflow = 4,
    BoundRangeExceeded = 5,
    InvalidOpcode = 6,
    DeviceNotAvailable = 7,
    DoubleFault = 8,
    CoprocessorSegmentOverrun = 9,
    InvalidTSS = 10,
    SegmentNotPresent = 11,
    StackFault = 12,
    GeneralProtectionFault = 13,
    PageFault = 14,
    x87FPUFloatingPointError = 16,
    AlignmentCheck = 17,
    MachineCheck = 18,
    SIMDFloatingPointException = 19,
    VirtualizationException = 20,
    ControlProtectionException = 21,
};

// Both x86 and x86_64 share the same interrupt descriptor flags
constexpr u8 INTERRUPT_GATE = 0x8E;
constexpr u8 TRAP_GATE = 0x8F;

void set_interrupt_handler(u32 interrupt, uintptr_t handler, u8 flags);


class InterruptDisabler {
public:
    InterruptDisabler() {
        asm volatile("cli");
    }

    ~InterruptDisabler() {
        enable();
    }

    void enable() {
        asm volatile("sti");
    }
};

}