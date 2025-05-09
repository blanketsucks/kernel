#pragma once

#include <kernel/common.h>

namespace kernel::uhci {

struct FrameListEntry {
    u8 enable : 1;
    u8 type : 1;
    u8 : 2;
    u32 address : 28;
};

struct QueueHead {
    FrameListEntry horizontal_pointer;
    FrameListEntry vertical_pointer;
};

struct TransferDescriptor {
    u32 next;
    u32 status;
    u32 packet_header;
    u32 buffer_address;
    u64 system_use[2];
};

enum Command {
    RS = (1 << 0),      // Run/Stop
    HCRESET = (1 << 1), // Host Controller Reset
    GRESET = (1 << 2),  // Global Reset
    EGSM = (1 << 3),    // Global Suspend
    FGR = (1 << 4),     // Force Global Resume
    SWDBG = (1 << 5),   // Software Debug
    CF = (1 << 6),      // Configure Flag
    MPS = (1 << 7)      // Max Packet Size
};

enum Status {
    USBINT = (1 << 0),
    USB_ERROR_INTERRUPT = (1 << 1),
    RESUME_DETECT = (1 << 2),
    HOST_SYSTEM_ERROR = (1 << 3),
    HOS_CONTROLLER_PROCESS_ERROR = (1 << 4),
    HCHALTED = (1 << 5)
};

enum InterruptEnable {
    TIMEOUT_CRC = (1 << 0),
    RESUME = (1 << 1),
    COMPLETE_TRANSFER = (1 << 2),
    SHORT_PACKET = (1 << 3)
};

enum PortSC {
    CONNECT_STATUS = (1 << 0),
    CONNECT_STATUS_CHANGE = (1 << 1),
    PORT_ENABLED = (1 << 2),
    PORT_ENABLED_CHANGE = (1 << 3),
    LINE_STATUS = (1 << 4), // 2 bits
    RESUME_DETECTED = (1 << 6),
    LOW_SPEED_DEVICE = (1 << 8),
    PORT_RESET = (1 << 9),
    SUSPEND = (1 << 12),
};

enum IORegister {
    USBCMD = 0x00,
    USBSTS = 0x02,
    USBINTR = 0x04,
    FRNUM = 0x06,
    FRBASEADD = 0x08,
    SOFMOD = 0x0C,
    PORTSC1 = 0x10,
    PORTSC2 = 0x12
};

}