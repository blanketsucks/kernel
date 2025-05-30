#pragma once

#include <kernel/usb/controller.h>
#include <kernel/usb/ohci/ohci.h>
#include <kernel/arch/irq.h>
#include <kernel/pci/address.h>
#include <kernel/process/blocker.h>

#include <std/stack.h>

namespace kernel::usb {

class OHCIController : public Controller, public IRQHandler {
public:
    static OHCIController* create();

    u8 allocate_device_address() override { return m_device_address++; };

    size_t submit_control_transfer(Pipe*, const DeviceRequest&, PhysicalAddress, size_t) override;

private:
    struct Chain {
        ohci::TransferDescriptor* head;
        ohci::TransferDescriptor* tail;
    };

    OHCIController(pci::Address);

    void handle_irq() override;

    void handle_hub_status_change();

    void reset();
    void initialize();
    void initialize_resources();

    void create_endpoint_descriptors();
    void create_transfer_descriptors();

    ohci::EndpointDescriptor* allocate_endpoint_descriptor();
    ohci::TransferDescriptor* allocate_transfer_descriptor();

    ohci::TransferDescriptor* create_transfer_descriptor(Pipe*, ohci::PacketPID direction);

    Chain create_transfer_chain(Pipe*, ohci::PacketPID direction, PhysicalAddress buffer, size_t size);
    void free_transfer_chain(ohci::TransferDescriptor* head);

    ohci::EndpointDescriptor* create_endpoint_descriptor(Pipe* pipe, Chain chain);

    void enqueue_endpoint_descriptor(
        ohci::EndpointDescriptor* ed, ohci::EndpointDescriptor* anchor, ohci::EndpointDescriptor*& head
    );
    void dequeue_endpoint_descriptor(ohci::EndpointDescriptor* ed, ohci::EndpointDescriptor*& head);

    void enqueue_control_transfer(ohci::EndpointDescriptor*);
    void dequeue_control_transfer(ohci::EndpointDescriptor*);

    ohci::Registers* m_registers;
    ohci::HCCA* m_hcca;

    u8 m_port_count = 0;
    u8 m_device_address = 1;

    ohci::EndpointDescriptor* m_endpoint_descriptors;
    ohci::TransferDescriptor* m_transfer_descriptors;

    std::Stack<ohci::EndpointDescriptor*> m_free_eds;
    std::Stack<ohci::TransferDescriptor*> m_free_tds;

    ohci::EndpointDescriptor* m_control_ed = nullptr;
    ohci::EndpointDescriptor* m_control_head = nullptr;

    ohci::EndpointDescriptor* m_bulk_ed = nullptr;
};

}