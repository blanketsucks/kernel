#pragma once

#include <kernel/usb/uhci/uhci.h>
#include <kernel/usb/controller.h>
#include <kernel/usb/descriptor_pool.h>
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
    size_t submit_bulk_transfer(Pipe*, PhysicalAddress buffer, size_t length) override;

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

    void setup_transfer();

    uhci::TransferDescriptor* create_transfer_descriptor(Pipe*, uhci::PacketType direction, size_t size);
    TDChain create_td_chain(Pipe*, uhci::PacketType direction, u32 buffer, size_t size);

    io::Port m_port;
    Array<RefPtr<Device>, 2> m_devices;

    u8 m_device_address = 1;

    uhci::FrameListEntry* m_frame_list;

    OwnPtr<DescriptorPool<uhci::QueueHead>> m_qh_pool;
    OwnPtr<DescriptorPool<uhci::TransferDescriptor>> m_td_pool;

    uhci::TransferDescriptor* m_iso_tds;

    uhci::QueueHead* m_anchor_qh;

    uhci::QueueHead* m_iso_anchor_qh;
    uhci::QueueHead* m_interrupt_anchor_qh;
    uhci::QueueHead* m_control_anchor_qh;
    uhci::QueueHead* m_bulk_anchor_qh;

    BooleanBlocker m_irq_blocker;
};

}