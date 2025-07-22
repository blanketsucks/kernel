#include <kernel/usb/pipe.h>
#include <kernel/usb/device.h>
#include <kernel/usb/controller.h>
#include <kernel/usb/usb.h>
#include <kernel/memory/manager.h>

#include <std/bytes_buffer.h>

namespace kernel::usb {

OwnPtr<ControlPipe> ControlPipe::create(Device* device, u8 endpoint, u8 max_packet_size) {
    return OwnPtr<ControlPipe>(new ControlPipe(device, endpoint, max_packet_size));
}

ControlPipe::ControlPipe(
    Device* device, u8 endpoint, u8 max_packet_size
) : Pipe(device, Pipe::In, Pipe::Control, endpoint, max_packet_size) {
    m_buffer = reinterpret_cast<u8*>(MUST(MM->allocate_dma_region(PAGE_SIZE)));
}

void ControlPipe::submit_transfer(u8 request_type, u8 req, u16 value, u16 index, u16 length, void* data) {
    if (length > PAGE_SIZE) {
        return;
    }

    auto* controller = m_device->controller();
    auto buffer = std::BytesBuffer(m_buffer, PAGE_SIZE);

    auto* request = buffer.read<DeviceRequest>();

    request->request_type = request_type;
    request->request = req;
    request->value = value;
    request->index = index;
    request->length = length;

    PhysicalAddress address = MM->get_physical_address(m_buffer);
    controller->submit_control_transfer(this, *request, address, length);

    if (length) {
        memcpy(data, m_buffer + sizeof(DeviceRequest), length);
    }
}

OwnPtr<BulkPipe> BulkPipe::create(Device* device, Direction direction, u8 endpoint, u8 max_packet_size) {
    return OwnPtr<BulkPipe>(new BulkPipe(device, direction, endpoint, max_packet_size));
}

BulkPipe::BulkPipe(
    Device* device, Direction direction, u8 endpoint, u8 max_packet_size
) : Pipe(device, direction, Pipe::Bulk, endpoint, max_packet_size) {
    m_buffer = reinterpret_cast<u8*>(MUST(MM->allocate_dma_region(PAGE_SIZE)));
}

void BulkPipe::submit_transfer(void* data, size_t length) {
    if (length > PAGE_SIZE) {
        return;
    }

    auto* controller = m_device->controller();
    PhysicalAddress address = MM->get_physical_address(m_buffer);

    if (m_direction == Pipe::Out) {
        memcpy(m_buffer, data, length);
    }

    controller->submit_bulk_transfer(this, address, length);
    if (m_direction == Pipe::In) {
        memcpy(data, m_buffer, length);
    }
}


}