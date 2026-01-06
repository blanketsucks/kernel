#include <kernel/usb/device.h>
#include <kernel/usb/controller.h>
#include <kernel/usb/usb.h>

#include <std/format.h>

namespace kernel::usb {

RefPtr<Device> Device::create(Controller* controller, u8 port, u8 address) {
    return RefPtr<Device>(new Device(controller, port, address));
}

void Device::initialize() {
    m_default_control_pipe = ControlPipe::create(this, 0, 8);
    if (!m_default_control_pipe) {
        return;
    }

    DeviceDescriptor descriptor;
    this->submit_control_transfer(
        RequestType::DeviceToHost | RequestType::Standard | RequestType::Device,
        Request::GetDescriptor, USB_DESCRIPTOR_TYPE_DEVICE << 8, 0, sizeof(DeviceDescriptor), &descriptor
    );

    if (descriptor.length != sizeof(DeviceDescriptor)) {
        dbgln("Invalid device descriptor length: {:#x}", descriptor.length);
        return;
    }

    m_default_control_pipe->set_max_packet_size(descriptor.max_packet_size);

    u8 address = m_controller->allocate_device_address();
    this->set_device_address(address);

    dbgln("USB Device Descriptor (port {}):", m_port);
    dbgln(" - Length: {}", descriptor.length);
    dbgln(" - Type: {}", descriptor.type);
    dbgln(" - USB Version: {}.{}", descriptor.usb_version >> 8, descriptor.usb_version & 0xFF);
    dbgln(" - Class: {}", descriptor.device_class);
    dbgln(" - Subclass: {}", descriptor.device_subclass);
    dbgln(" - Protocol: {}", descriptor.device_protocol);
    dbgln(" - Max Packet Size: {}", descriptor.max_packet_size);
    dbgln(" - Vendor ID: {:#x}", descriptor.vendor_id);
    dbgln(" - Product ID: {:#x}", descriptor.product_id);
    dbgln();
}

size_t Device::submit_control_transfer(RequestType type, u8 request, u16 value, u16 index, u16 length, void* data) {
    if (!m_default_control_pipe) {
        return 0;
    }

    m_default_control_pipe->submit_transfer(
        to_underlying(type), request, value, index, length, data
    );

    return length;
}

String Device::fetch_string_descriptor(u8 index, u16 lang_id) {
    UnicodeStringDescriptor descriptor;
    this->submit_control_transfer(
        RequestType::DeviceToHost | RequestType::Standard | RequestType::Device,
        Request::GetDescriptor, 
        (USB_DESCRIPTOR_TYPE_STRING << 8) | index, 
        lang_id, 
        sizeof(UnicodeStringDescriptor), 
        &descriptor
    );

    u8* buffer = new u8[descriptor.length];
    this->submit_control_transfer(
        RequestType::DeviceToHost | RequestType::Standard | RequestType::Device,
        Request::GetDescriptor,
        (USB_DESCRIPTOR_TYPE_STRING << 8) | index,
        lang_id,
        descriptor.length,
        buffer
    );

    size_t len = (descriptor.length - 2) / 2;

    String str;
    str.reserve(len);

    // TODO: Support unicode strings
    for (size_t i = 0; i < len; i++) {
        char16_t ch = reinterpret_cast<char16_t*>(buffer + 2)[i];
        str.append(static_cast<char>(ch));
    }

    delete[] buffer;
    return str;
}

void Device::set_device_address(u8 address) {
    if (m_address != 0) {
        return;
    }

    this->submit_control_transfer(
        RequestType::HostToDevice | RequestType::Standard | RequestType::Device,
        Request::SetAddress, address, 0, 0, nullptr
    );

    m_address = address;
}

}

