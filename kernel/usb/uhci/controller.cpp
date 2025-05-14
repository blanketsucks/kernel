#include <kernel/usb/uhci/controller.h>
#include <kernel/usb/pipe.h>
#include <kernel/usb/usb.h>
#include <kernel/usb/device.h>
#include <kernel/memory/manager.h>
#include <kernel/arch/io.h>
#include <kernel/process/process.h>
#include <kernel/process/scheduler.h>

#include <std/format.h>

namespace kernel::usb {

using namespace uhci;

static constexpr size_t ISO_TD_COUNT = PAGE_SIZE / sizeof(TransferDescriptor);

UHCIController* UHCIController::create() {
    pci::Address address;
    pci::enumerate([&address](pci::Device device) {
        if (device.is_usb_controller()) {
            u8 prog_if = device.address().prog_if();
            if (prog_if != 0x00) {
                return;
            }

            address = device.address();
        }
    });

    if (!address.value()) {
        return nullptr;
    }

    auto controller = new UHCIController(address);
    if (!controller) {
        return nullptr;
    }

    return controller;
}

UHCIController::UHCIController(pci::Address address) : IRQHandler(address.interrupt_line()) {
    dbgln();

    address.set_bus_master(true);
    address.set_interrupt_line(true);

    m_port = address.bar(4) & ~1;
    this->initialize();

    dbgln();
}

void UHCIController::reset() {
    m_port.write<u16>(IORegister::USBCMD, Command::HCRESET);
    while (m_port.read<u16>(IORegister::USBCMD) & Command::HCRESET) {}
}

void UHCIController::initialize() {
    this->reset();

    this->create_resources();
    this->setup_transfer();

    m_port.write<u8>(IORegister::SOFMOD, 64); // 1ms
    m_port.write<u32>(IORegister::FRBASEADD, MM->get_physical_address(m_frame_list));
    m_port.write<u16>(IORegister::FRNUM, 0);

    m_port.write<u16>(IORegister::USBINTR, InterruptEnable::COMPLETE_TRANSFER | InterruptEnable::TIMEOUT_CRC);
    m_port.write<u16>(IORegister::USBCMD, Command::RS | Command::MPS);

    this->spawn_poll_process();
    this->enable_irq();
}

void UHCIController::create_resources() {
    m_frame_list = reinterpret_cast<FrameListEntry*>(MM->allocate_dma_region(PAGE_SIZE));
    for (size_t i = 0; i < FRAME_COUNT; i++) {
        m_frame_list[i].enable = 0;
    }
    
    this->create_queue_heads();
    this->create_transfer_descriptors();
}

void UHCIController::spawn_poll_process() {
    auto* process = Process::create_kernel_process("UHCI Poll", [this]() {
        this->poll_connected_ports();
    });

    Scheduler::add_process(process);
}

void UHCIController::poll_connected_ports() {
    auto* thread = Thread::current();

    while (true) {
        for (size_t i = 0; i < 2; i++) {
            u16 portsc = m_port.read<u16>(IORegister::PORTSC1 + i * 2);
            if (!(portsc & PortSC::CONNECT_STATUS)) {
                continue;
            } else if (portsc & PortSC::PORT_ENABLED) {
                continue;
            }

            this->enable_port(i);
        }

        thread->sleep(1);
    }
}

void UHCIController::enable_port(u8 port) {
    u16 offset = IORegister::PORTSC1 + port * 2;

    u16 portsc = m_port.read<u16>(offset);
    m_port.write<u16>(offset, portsc | PortSC::PORT_RESET);

    io::wait(100);
    m_port.write<u16>(offset, portsc & ~PortSC::PORT_RESET);

    io::wait(50);
    m_port.write<u16>(offset, portsc | PortSC::PORT_ENABLED);

    io::wait(100);

    auto device = Device::create(this, port);
    device->initialize();

    m_devices[port] = device;
}

void UHCIController::create_queue_heads() {
    m_queue_heads = reinterpret_cast<QueueHead*>(MM->allocate_kernel_region(PAGE_SIZE));
    PhysicalAddress address = MM->get_physical_address(m_queue_heads);

    for (size_t i = 0; i < PAGE_SIZE / sizeof(QueueHead); i++) {
        auto* qh = &m_queue_heads[i];
        new (qh) QueueHead(address + i * sizeof(QueueHead));

        m_free_qhs.push(qh);
    }
}

void UHCIController::create_transfer_descriptors() {
    m_transfer_descriptors = reinterpret_cast<TransferDescriptor*>(MM->allocate_kernel_region(PAGE_SIZE));
    PhysicalAddress address = MM->get_physical_address(m_transfer_descriptors);

    for (size_t i = 0; i < PAGE_SIZE / sizeof(TransferDescriptor); i++) {
        auto* td = &m_transfer_descriptors[i];
        new (td) TransferDescriptor(address + i * sizeof(TransferDescriptor));

        m_free_tds.push(td);
    }
}

uhci::QueueHead* UHCIController::allocate_queue_head() {
    if (m_free_qhs.empty()) {
        return nullptr;
    }

    return m_free_qhs.pop();
}

uhci::TransferDescriptor* UHCIController::allocate_transfer_descriptor() {
    if (m_free_tds.empty()) {
        return nullptr;
    }

    return m_free_tds.pop();
}

void UHCIController::setup_transfer() {
    // We will essentially structure the queue heads by order of priority.
    m_anchor_qh = this->allocate_queue_head();

    m_iso_anchor_qh = this->allocate_queue_head();
    m_interrupt_anchor_qh = this->allocate_queue_head();
    m_control_anchor_qh = this->allocate_queue_head();
    m_bulk_anchor_qh = this->allocate_queue_head();

    m_anchor_qh->link(m_iso_anchor_qh);
    m_anchor_qh->terminate_element_link();

    m_iso_anchor_qh->link(m_interrupt_anchor_qh);
    m_iso_anchor_qh->terminate_element_link();

    m_interrupt_anchor_qh->link(m_control_anchor_qh);
    m_interrupt_anchor_qh->terminate_element_link();

    m_control_anchor_qh->link(m_bulk_anchor_qh);
    m_control_anchor_qh->terminate_element_link();

    m_iso_tds = reinterpret_cast<TransferDescriptor*>(MM->allocate_dma_region(PAGE_SIZE));
    PhysicalAddress address = MM->get_physical_address(m_iso_tds);

    for (size_t i = 0; i < ISO_TD_COUNT; i++) {
        auto* td = &m_iso_tds[i];
        new (td) TransferDescriptor(address + i * sizeof(TransferDescriptor));

        td->set_isochronous();
        td->link(m_anchor_qh->address());
    }

    for (size_t i = 0; i < FRAME_COUNT; i++) {
        auto& entry = m_frame_list[i];
        auto& td = m_iso_tds[i % ISO_TD_COUNT];

        *reinterpret_cast<u32*>(&entry) = td.address();
    }
}

TransferDescriptor* UHCIController::create_transfer_descriptor(Pipe* pipe, uhci::PacketType direction, size_t size) {
    auto* td = this->allocate_transfer_descriptor();
    if (!td) {
        return nullptr;
    }

    td->set_packet_type(direction);
    if (size) {
        td->set_max_length(size - 1);
    } else {
        td->set_max_length(0x7ff);
    }

    td->set_data_toggle(pipe->data_toggle());
    pipe->set_data_toggle(!pipe->data_toggle());

    if (pipe->type() == Pipe::Isochronous) {
        td->set_isochronous();
    } else if (direction == PacketType::In) {
        td->set_short_packet();
    }

    td->set_active();

    td->set_endpoint(pipe->endpoint());
    td->set_device_address(pipe->device()->address());

    return td;
}

auto UHCIController::create_td_chain(Pipe* pipe, uhci::PacketType direction, u32 buffer, size_t size) -> TDChain {
    size_t offset = 0;

    TransferDescriptor* head = nullptr;
    TransferDescriptor* tail = nullptr;

    u8 max_packet_size = pipe->max_packet_size();
    while (offset < size) {
        size_t packet_size = std::min(size - offset, static_cast<size_t>(max_packet_size));
        auto* td = this->create_transfer_descriptor(pipe, direction, packet_size);

        td->set_buffer_address(buffer + offset);
        if (tail) {
            tail->link(td);
        } else {
            head = td;
        }

        offset += packet_size;
        tail = td;
    }

    return { head, tail };
}

size_t UHCIController::submit_control_transfer(Pipe* pipe, const DeviceRequest& request, PhysicalAddress buffer, size_t length) {
    m_irq_blocker.set_value(false);
    pipe->set_data_toggle(false);

    bool is_device_to_host = (request.request_type & (u8)RequestType::DeviceToHost) != 0;

    auto* setup = this->create_transfer_descriptor(pipe, PacketType::Setup, sizeof(DeviceRequest));
    setup->set_buffer_address(buffer);

    auto chain = this->create_td_chain(pipe, is_device_to_host ? PacketType::In : PacketType::Out, buffer + sizeof(DeviceRequest), length);

    pipe->set_data_toggle(true);
    auto* status = this->create_transfer_descriptor(pipe, is_device_to_host ? PacketType::Out : PacketType::In, 0);

    status->terminate();
    status->set_interrupt_on_complete();

    if (chain.head) {
        setup->link(chain.head);
        chain.tail->link(status);
    } else {
        setup->link(status);
    }

    auto* qh = this->allocate_queue_head();
    qh->attach_td(setup);

    auto* prev = m_control_anchor_qh->prev();
    prev->link(qh);
    qh->link(m_anchor_qh);

    m_irq_blocker.wait();
    return 0;
}

void UHCIController::handle_irq() {
    u16 status = m_port.read<u16>(IORegister::USBSTS);
    if (!status) {
        return;
    }

    if (status & Status::USB_ERROR_INTERRUPT) {
        dbgln("Error occurred during transfer: {:#x}", status);
    }
    
    if (status & Status::USBINT) {
        m_irq_blocker.set_value(true);
    }

    m_port.write<u16>(IORegister::USBSTS, status);
}

}