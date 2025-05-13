#pragma once

#include <kernel/common.h>

namespace kernel::uhci {

constexpr size_t FRAME_COUNT = 1024;

struct FrameListEntry {
    u8 enable : 1;
    u8 type : 1;
    u8 : 2;
    u32 address : 28;
};

enum PacketType {
    In = 0x69,
    Out = 0xE1,
    Setup = 0x2D  
};

class TransferDescriptor {
public:
    struct Data {
        u32 address;
        TransferDescriptor* next;
        TransferDescriptor* prev;
    };

    enum Status : u32 {
        ActualLength = (1 << 0),
        BitStuffError = (1 << 17),
        TimeoutCRC = (1 << 18),
        NonAcknowledged = (1 << 19),
        BabbleDetected = (1 << 20),
        DataBufferError = (1 << 21),
        Stalled = (1 << 22),
        Active = (1 << 23),
        InterruptOnComplete = (1 << 24),
        IsIsochronous = (1 << 25),
        LowSpeed = (1 << 26),
        ErrorCount = (1 << 27),
        ShortPacketDetect = (1 << 29)
    };

    enum PacketHeader : u32 {
        PacketType = (1 << 0),
        Device = (1 << 8),
        Endpoint = (1 << 15),
        DataToggle = (1 << 19),
        MaxLength = (1 << 21)
    };

    enum {
        Terminate           = (1 << 0), // 0 = Pointer is valid 1 = This TD does not point do anything
        MemoryStructureType = (1 << 1), // 0 = Transfer Descriptor 1 = Queue Head
        DepthFirst          = (1 << 2) // Controller will continue execution to next TD pointed by this TD
    };

    TransferDescriptor(PhysicalAddress address) : m_next_address(0), m_status(0), m_packet_header(0) {
        m_data = new Data;
        m_data->address = address;
    }

    u32 address() const { return m_data->address; }
    u32 status() const { return m_status; }
    u32 packet_header() const { return m_packet_header; }
    u32 buffer_address() const { return m_buffer_address; }

    void set_packet_type(enum PacketType type) { m_packet_header = type; }
    void set_device_address(u8 address) { m_packet_header |= address << 8; }
    void set_endpoint(u8 endpoint) { m_packet_header |= endpoint << 15; }
    void set_data_toggle(bool toggle) { m_packet_header |= toggle << 19; }
    void set_max_length(u16 length) { m_packet_header |= length << 21; }

    void set_isochronous() {
        m_status |= Status::IsIsochronous;
    }

    void set_short_packet() {
        m_status |= Status::ShortPacketDetect;
    }

    void set_interrupt_on_complete() {
        m_status |= Status::InterruptOnComplete;
    }

    void set_active() {
        m_status |= Status::Active;
    }

    void set_error_count(u8 count) {
        m_status |= count << 27;
    }

    TransferDescriptor* next() const {
        return m_data->next;
    }

    void set_next(TransferDescriptor* next) {
        m_data->next = next;
    }

    TransferDescriptor* prev() const {
        return m_data->prev;
    }

    void set_prev(TransferDescriptor* prev) {
        m_data->prev = prev;
    }

    void set_buffer_address(PhysicalAddress address) {
        m_buffer_address = address;
    }

    void link(TransferDescriptor* next) {
        m_next_address = next->address();

        m_data->next = next;
        next->m_data->prev = this;

        m_next_address |= DepthFirst;
    }

    void link(u32 qh) {
        m_next_address = qh | MemoryStructureType;
    }

    void terminate() {
        m_next_address |= Terminate;
    }

private:
    u32 m_next_address;
    u32 m_status;
    u32 m_packet_header;
    u32 m_buffer_address;

    Data* m_data;
    [[gnu::unused]] u8 padding[sizeof(u64) * 2 - sizeof(Data*)];
} PACKED;

class QueueHead {
public:
    struct Data {
        u32 address;
        QueueHead* next;
        QueueHead* prev;

        TransferDescriptor* td; // First TD
    };

    enum {
        Terminate           = (1 << 0), // 0 = Pointer is valid 1 = This TD does not point do anything
        MemoryStructureType = (1 << 1), // 0 = Transfer Descriptor 1 = Queue Head
        DepthFirst          = (1 << 2) // Controller will continue execution to next TD pointed by this TD
    };

    QueueHead(PhysicalAddress address) : m_link(0), m_element(0) {
        m_data = new Data;
        m_data->address = address;
    }

    u32 address() const { return m_data->address; }

    QueueHead* next() const {
        return m_data->next;
    }

    QueueHead* prev() const {
        return m_data->prev;
    }

    TransferDescriptor* td() const {
        return m_data->td;
    }

    void link(QueueHead* next) {
        m_link = next->address();

        m_data->next = next;
        next->m_data->prev = this;

        m_link |= MemoryStructureType;
    }

    void attach_td(TransferDescriptor* td) {
        m_data->td = td;
        m_element = td->address();
    }

    void attach_qh(QueueHead* next) {
        m_element = next->address() | MemoryStructureType;
    }

    void terminate() { m_link |= Terminate; }
    void terminate_element_link() { m_element = Terminate; }

private:
    u32 m_link;     // Horizontal link
    u32 m_element;  // Vertical link

    Data* m_data;
    [[gnu::unused]] u8 padding[sizeof(TransferDescriptor) - sizeof(u32) * 2 - sizeof(Data*)];
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
    RESERVED = (1 << 7),
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