#pragma once

#include <kernel/usb/uhci/uhci.h>
#include <kernel/usb/controller.h>
#include <kernel/usb/device.h>
#include <kernel/arch/io.h>
#include <kernel/pci/pci.h>
#include <kernel/arch/irq.h>
#include <kernel/process/blocker.h>

#include <std/stack.h>
#include <std/array.h>

namespace kernel::usb {

class UHCIController : public Controller, public IRQHandler {
public:
    static UHCIController* create();

    void initialize();
    void reset();

    void create_resources();

    size_t submit_control_transfer(Pipe*, const DeviceRequest& request, PhysicalAddress buffer, size_t length) override;
    size_t submit_bulk_transfer(Pipe*, PhysicalAddress, size_t) override { return 0; }

    u8 allocate_device_address() override {
        return m_device_address++;
    }

private:
    struct TDChain {
        uhci::TransferDescriptor* head;
        uhci::TransferDescriptor* tail;
    };

    UHCIController(pci::Address);

    void spawn_poll_process();

    void poll_connected_ports();
    void enable_port(u8 port);

    void handle_irq() override;

    void create_queue_heads();
    void create_transfer_descriptors();

    uhci::QueueHead* allocate_queue_head();
    uhci::TransferDescriptor* allocate_transfer_descriptor();

    void setup_transfer();

    uhci::TransferDescriptor* create_transfer_descriptor(Pipe*, uhci::PacketType direction, size_t size);
    TDChain create_td_chain(Pipe*, uhci::PacketType direction, u32 buffer, size_t size);

    io::Port m_port;
    Array<RefPtr<Device>, 2> m_devices;

    u8 m_device_address = 1;

    uhci::FrameListEntry* m_frame_list;

    uhci::QueueHead* m_queue_heads;
    uhci::TransferDescriptor* m_transfer_descriptors;

    std::Stack<uhci::QueueHead*> m_free_qhs;
    std::Stack<uhci::TransferDescriptor*> m_free_tds;

    uhci::TransferDescriptor* m_iso_tds;

    uhci::QueueHead* m_anchor_qh;

    uhci::QueueHead* m_iso_anchor_qh;
    uhci::QueueHead* m_interrupt_anchor_qh;
    uhci::QueueHead* m_control_anchor_qh;
    uhci::QueueHead* m_bulk_anchor_qh;

    BooleanBlocker m_irq_blocker;
};

}