#pragma once

#include <kernel/common.h>

namespace kernel::ohci {

enum InterruptEnable : u32 {
    SchedulingOverrun = 1 << 0,
    WritebackDoneHead = 1 << 1,
    StartOfFrame = 1 << 2,
    ResumeDetect = 1 << 3,
    UnrecoverableError = 1 << 4,
    FrameNumberOverflow = 1 << 5,
    RootHubStatusChange = 1 << 6,
    MasterInterruptEnable = 1ul << 31,

    AllInterrupts = SchedulingOverrun | WritebackDoneHead | StartOfFrame | ResumeDetect | UnrecoverableError | FrameNumberOverflow | RootHubStatusChange,
    Desired = WritebackDoneHead | RootHubStatusChange | UnrecoverableError | MasterInterruptEnable
};

enum Control {
    PeriodicListEnable = 1 << 2,
    IsochronousEnable = 1 << 3,
    ControlListEnable = 1 << 4,
    BulkListEnable = 1 << 5,
    InterruptRouting = 1 << 8,

    AllListsEnable = PeriodicListEnable | IsochronousEnable | ControlListEnable | BulkListEnable,
    FunctionalStateMask = 0b11 << 6,
    ServiceRatioMask = 0b11
};

enum FunctionalState {
    Reset = 0 << 6,
    Resume = 1 << 6,
    Operational = 2 << 6,
    Suspend = 3 << 6
};

enum CommandStatus {
    HostControllerReset = 1 << 0,
    ControlListFilled = 1 << 1,
    BulkListFilled = 1 << 2,
    OwnershipChangeRequest = 1 << 3
};

enum RootHubPortStatus : u32 {
    ConnectStatus = 1 << 0,
    EnableStatus = 1 << 1,
    SuspendStatus = 1 << 2,
    OverCurrentIndicator = 1 << 3,
    ResetStatus = 1 << 4,
    PowerStatus = 1 << 8,
    LowSpeedDevice = 1 << 9,
    ConnectStatusChange = 1 << 16,
    EnableChange = 1 << 17,
    SuspendChange = 1 << 18,
    OverCurrentChange = 1 << 19,
    ResetChange = 1 << 20
};

struct Registers {
    u32 revision;
    u32 control;
    u32 command_status;
    u32 interrupt_status;
    u32 interrupt_enable;
    u32 interrupt_disable;
    u32 hcca;
    u32 period_current_ed;
    u32 control_head_ed;
    u32 control_current_ed;
    u32 bulk_head_ed;
    u32 bulk_current_ed;
    u32 done_head;
    u32 frame_interval;
    u32 frame_remaining;
    u32 frame_number;
    u32 periodic_start;
    u32 lsthreshold;
    u32 root_hub_descriptor_a;
    u32 root_hub_descriptor_b;
    u32 root_hub_status;
    u32 ports[0];
} PACKED;

struct HCCA {
    u32 interrupt_table[32];
    u16 frame_number;
    u16 pad;
    u32 done_head;
} PACKED;

enum PacketPID {
    Setup = 0b00,
    Out = 0b01,
    In = 0b10
};

class ALIGNED(16) TransferDescriptor {
public:
    struct Data {
        TransferDescriptor* next = nullptr;
        PhysicalAddress address;
    };

    enum Control {
        BufferRounding = 1 << 18,
        Direction = 1 << 19,
        DelayInterrupt = 1 << 21,
        DataToggle = 1 << 24,
        ErrorCount = 1 << 26,
        ConditionCode = 1 << 28
    };

    TransferDescriptor(PhysicalAddress address) {
        m_data = new Data();
        m_data->address = address;
    }

    PhysicalAddress address() const {
        return m_data->address;
    }

    TransferDescriptor* next() const {
        return m_data->next;
    }

    u32 control() const {
        return m_control;
    }

    u8 condition_code() const {
        return (m_control >> 28) & 0x0F;
    }

    void set_direction(u8 direction) {
        m_control |= (direction & 0x03) << 19;
    }

    void set_data_toggle(u8 toggle) {
        m_control |= (toggle & 0x03) << 24;
    }

    void set_interrupt_delay(u8 delay) {
        m_control |= (delay & 0x07) << 21;
    }

    void set_buffer_address(PhysicalAddress address, size_t size) {
        if (!address) {
            m_current_buffer_pointer = 0;
            m_buffer_end = 0;
            
            return;
        }

        m_current_buffer_pointer = address;
        m_buffer_end = address + size - 1;
    }

    void set_next(TransferDescriptor* next) {
        m_data->next = next;
        m_next = next ? next->address() : 0;
    }

private:
    u32 m_control;
    u32 m_current_buffer_pointer;
    u32 m_next;
    u32 m_buffer_end;

    Data* m_data;
};

class ALIGNED(16) EndpointDescriptor {
public:
    struct Data {
        TransferDescriptor* head = nullptr;
        TransferDescriptor* tail = nullptr;

        EndpointDescriptor* next = nullptr;
        EndpointDescriptor* prev = nullptr;

        PhysicalAddress address;
    };

    enum Control {
        FunctionAddress = 1 << 0,
        Endpoint = 1 << 7,
        Direction = 1 << 11,
        Speed = 1 << 13,
        Skip = 1 << 14,
        Format = 1 << 15,
        MaxPacketSize = 1 << 16
    };

    EndpointDescriptor(PhysicalAddress address) {
        m_data = new Data();
        m_data->address = address;
    }

    PhysicalAddress address() const {
        return m_data->address;
    }

    Data* data() const {
        return m_data;
    }

    u32 control() const { return m_control; }
    u32 head() const { return m_head; }
    u32 tail() const { return m_tail; }

    void set_low_speed() { m_control |= Speed; }
    void set_skip() { m_control |= Skip; }

    void set_function_address(u8 address) {
        m_control |= address & 0x7F;
    }

    void set_endpoint(u8 endpoint) {
        m_control |= (endpoint & 0x0F) << 7;
    }

    void set_direction(u8 direction) {
        m_control |= (direction & 0x03) << 11;
    }

    void set_max_packet_size(u16 size) {
        m_control |= (size & 0x7FF) << 16;
    }

    void set_tail(TransferDescriptor* tail) {
        m_data->tail = tail;
        m_tail = tail ? tail->address() : 0;
    }

    void set_head(TransferDescriptor* head) {
        m_data->head = head;
        m_head = head ? head->address() : 0;
    }

    void set_next(EndpointDescriptor* next) {
        m_data->next = next;
        if (next) {
            next->m_data->prev = this;
            m_next = next->address();
        } else {
            m_next = 0;
        }
    }

public:
    u32 m_control;
    u32 m_tail = 0;
    u32 m_head = 0;
    u32 m_next = 0;
    
    Data* m_data;
};

}