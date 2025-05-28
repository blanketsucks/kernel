#include <kernel/usb/ohci/controller.h>
#include <kernel/usb/pipe.h>
#include <kernel/usb/device.h>
#include <kernel/pci/pci.h>
#include <kernel/memory/manager.h>
#include <kernel/arch/io.h>

#include <std/format.h>

namespace kernel::usb {

using namespace ohci;

static constexpr size_t ED_COUNT = PAGE_SIZE / sizeof(ohci::EndpointDescriptor);
static constexpr size_t TD_COUNT = PAGE_SIZE / sizeof(ohci::TransferDescriptor);

OHCIController* OHCIController::create() {
    pci::Address address;
    bool found = false;

    PCI::enumerate([&found, &address](pci::Device device) {
        if (device.is_usb_controller()) {
            u8 prog_if = device.address().prog_if();
            if (prog_if != 0x10) {
                return;
            }

            address = device.address();
            found = true;
        }
    });

    if (!found) {
        return nullptr;
    }

    auto controller = new OHCIController(address);
    if (!controller) {
        return nullptr;
    }

    return controller;
}

OHCIController::OHCIController(pci::Address address) : IRQHandler(address.interrupt_line()) {
    m_registers = reinterpret_cast<ohci::Registers*>(
        MM->map_physical_region(reinterpret_cast<void*>(address.bar(0) & ~3), PAGE_SIZE)
    );

    address.set_bus_master(true);
    address.set_interrupt_line(true);

    dbgln("OHCI Controller ({}:{}:{}):", address.bus(), address.device(), address.function());
    dbgln(" - Revision: {:#x}", m_registers->revision);
    m_port_count = m_registers->root_hub_descriptor_a & 0xff;
    dbgln(" - Ports: {}", m_port_count);
    dbgln();

    this->initialize();
    this->enable_irq();

    this->handle_hub_status_change();
}

void OHCIController::reset() {
    m_registers->control = FunctionalState::Reset;
    m_registers->command_status = CommandStatus::HostControllerReset;

    // TODO: Timeout
    while (m_registers->command_status & CommandStatus::HostControllerReset) {}
}

void OHCIController::initialize() {
    if (m_registers->control & Control::InterruptRouting) {
        // TODO: Handle ownership change
    }

    // The Host Controller Driver should now save the contents of the HcFmInterval register and then issue a software reset
    u32 frame_interval = m_registers->frame_interval;

    this->reset();
    this->initialize_resources();

    m_registers->interrupt_disable = InterruptEnable::AllInterrupts;
    m_registers->interrupt_enable = InterruptEnable::Desired;

    u32 control = m_registers->control;
    control &= ~(Control::AllListsEnable | Control::FunctionalStateMask | Control::InterruptRouting);

    control |= FunctionalState::Operational;
    control |= Control::AllListsEnable; // Set HcControl to have “all queues on”

    // TODO: Set up the interrupt EDs in the HCCA

    m_registers->control = control;

    // After the software reset is complete (a maximum of 10 ms), the Host Controller Driver should restore the value of the HcFmInterval register.
    m_registers->frame_interval = frame_interval;

    // Set HcPeriodicStart to a value that is 90% of the value in FrameInterval field of the HcFmInterval register.
    m_registers->periodic_start = (frame_interval * 9) / 10;
}

void OHCIController::initialize_resources() {
    m_hcca = reinterpret_cast<ohci::HCCA*>(MM->allocate_dma_region(sizeof(ohci::HCCA)));
    memset(m_hcca, 0, sizeof(ohci::HCCA));
    
    this->create_endpoint_descriptors();
    this->create_transfer_descriptors();

    m_control_ed = this->allocate_endpoint_descriptor();
    m_bulk_ed = this->allocate_endpoint_descriptor();

    m_control_ed->set_skip();
    m_bulk_ed->set_skip();

    m_registers->hcca = MM->get_physical_address(m_hcca);
    m_registers->control_head_ed = m_control_ed->address();
    m_registers->bulk_head_ed = m_bulk_ed->address();
}

void OHCIController::handle_irq() {
    u32 status = m_registers->interrupt_status;
    if (!status) {
        return;
    }

    if (status & InterruptEnable::RootHubStatusChange) {
        this->handle_hub_status_change();
    }

    m_registers->interrupt_status = status;
}

void OHCIController::handle_hub_status_change() {
    for (size_t i = 0; i < m_port_count; i++) {
        auto port = m_registers->ports[i];

        bool current_connect_status = port & RootHubPortStatus::ConnectStatus;
        bool connect_status_change = port & RootHubPortStatus::ConnectStatusChange;

        if (!current_connect_status || !connect_status_change) {
            continue;
        }
        
        if (port & RootHubPortStatus::EnableStatus) {
            continue;
        }
        
        port |= RootHubPortStatus::ResetStatus;
        m_registers->ports[i] = port;

        io::wait(10 * 1000);

        port &= ~RootHubPortStatus::ResetStatus;
        port |= RootHubPortStatus::EnableStatus;

        m_registers->ports[i] = port;
        io::wait(10 * 1000);

        auto device = Device::create(this, i + 1).take();
        device->initialize();
    }
}


void OHCIController::create_endpoint_descriptors() {
    m_endpoint_descriptors = reinterpret_cast<ohci::EndpointDescriptor*>(MM->allocate_kernel_region(PAGE_SIZE));
    PhysicalAddress address = MM->get_physical_address(m_endpoint_descriptors);

    for (size_t i = 0; i < ED_COUNT; i++) {
        auto* ed = &m_endpoint_descriptors[i];
        new (ed) ohci::EndpointDescriptor(address + i * sizeof(ohci::EndpointDescriptor));

        m_free_eds.push(ed);
    }
}

void OHCIController::create_transfer_descriptors() {
    m_transfer_descriptors = reinterpret_cast<ohci::TransferDescriptor*>(MM->allocate_kernel_region(PAGE_SIZE));
    PhysicalAddress address = MM->get_physical_address(m_transfer_descriptors);

    for (size_t i = 0; i < TD_COUNT; i++) {
        auto* td = &m_transfer_descriptors[i];
        new (td) ohci::TransferDescriptor(address + i * sizeof(ohci::TransferDescriptor));

        m_free_tds.push(td);
    }
}

ohci::EndpointDescriptor* OHCIController::allocate_endpoint_descriptor() {
    if (m_free_eds.empty()) {
        return nullptr;
    }

    return m_free_eds.pop();
}

ohci::TransferDescriptor* OHCIController::allocate_transfer_descriptor() {
    if (m_free_tds.empty()) {
        return nullptr;
    }

    return m_free_tds.pop();
}

void OHCIController::free_transfer_chain(TransferDescriptor* head) {
    TransferDescriptor* current = head;
    while (current) {
        auto* next = current->next();

        current->reset();
        m_free_tds.push(current);

        current = next;
    }
}

TransferDescriptor* OHCIController::create_transfer_descriptor(Pipe* pipe, ohci::PacketPID direction) {
    auto* td = this->allocate_transfer_descriptor();

    td->set_direction(direction);
    td->set_data_toggle(pipe->data_toggle());

    pipe->set_data_toggle(!pipe->data_toggle());
    return td;
}

OHCIController::Chain OHCIController::create_transfer_chain(Pipe* pipe, ohci::PacketPID direction, PhysicalAddress buffer, size_t size) {
    TransferDescriptor* head = nullptr;
    TransferDescriptor* tail = nullptr;

    u8 max_packet_size = pipe->max_packet_size();
    size_t offset = 0;

    while (offset < size) {
        size_t packet_size = std::min(size - offset, static_cast<size_t>(max_packet_size));
        auto* td = this->create_transfer_descriptor(pipe, direction);

        td->set_buffer_address(buffer + offset, packet_size);
        if (tail) {
            tail->set_next(td);
        } else {
            head = td;
        }

        offset += packet_size;
        tail = td;
    }

    return { head, tail };
}

EndpointDescriptor* OHCIController::create_endpoint_descriptor(Pipe* pipe, Chain chain) {
    auto* ed = this->allocate_endpoint_descriptor();
    if (!ed) {
        return nullptr;
    }

    ed->set_function_address(pipe->device()->address());
    ed->set_endpoint(pipe->endpoint());
    ed->set_direction(0); // Direction will be taken from the transfer descriptors
    ed->set_max_packet_size(pipe->max_packet_size());

    ed->set_head(chain.head);
    ed->set_tail(chain.tail);

    return ed;
}

void OHCIController::enqueue_control_transfer(EndpointDescriptor* ed) {
    if (!m_control_head) {
        m_control_head = ed;
        m_control_ed->set_next(m_control_head);
    } else {
        m_control_head->set_next(ed);
    }

    m_registers->command_status |= CommandStatus::ControlListFilled;
}

void OHCIController::dequeue_control_transfer(EndpointDescriptor* ed) {
    auto* prev = ed->prev();
    if (!ed->next()) {
        m_control_head = prev;
    }

    prev->set_next(ed->next());
}

size_t OHCIController::submit_control_transfer(Pipe* pipe, const DeviceRequest& request, PhysicalAddress buffer, size_t length) {
    pipe->set_data_toggle(false);
    bool is_device_to_host = request.is_device_to_host();

    auto* setup = this->create_transfer_descriptor(pipe, PacketPID::Setup);
    setup->set_buffer_address(buffer, sizeof(DeviceRequest));

    auto chain = this->create_transfer_chain(
        pipe, is_device_to_host ? PacketPID::In : PacketPID::Out, buffer + sizeof(DeviceRequest), length
    );

    pipe->set_data_toggle(true);
    auto* status = this->create_transfer_descriptor(pipe, is_device_to_host ? PacketPID::Out : PacketPID::In);

    auto* tail = this->allocate_transfer_descriptor();

    tail->set_buffer_address(0, 0);
    tail->set_next(nullptr);

    status->set_buffer_address(0, 0);
    status->set_next(tail);

    if (chain.head) {
        setup->set_next(chain.head);
        chain.tail->set_next(status);
    } else {
        setup->set_next(status);
    }

    auto* ed = this->create_endpoint_descriptor(pipe, { setup, tail });
    this->enqueue_control_transfer(ed);

    // TODO: Check for errors
    // FIXME: There is a chance this will block a kernel process indefinitely especially if this is done early on (kernel stage 2 process).
    while (true) {
        if ((ed->head() & ~0xf) == ed->tail()) {
            // The transfer is complete
            break;
        }
    }

    this->dequeue_control_transfer(ed);
    this->free_transfer_chain(setup);

    ed->reset();
    m_free_eds.push(ed);

    return length;
}


}