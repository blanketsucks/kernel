#pragma once

#include <kernel/common.h>

namespace kernel::usb {

static constexpr u16 USB_DESCRIPTOR_TYPE_DEVICE = 0x01;
static constexpr u16 USB_DESCRIPTOR_TYPE_CONFIGURATION = 0x02;
static constexpr u16 USB_DESCRIPTOR_TYPE_STRING = 0x03;

enum class RequestType : u8 {
    // Direction
    HostToDevice = (0 << 7),
    DeviceToHost = (1 << 7),

    // Type
    Standard = (0 << 5),
    Class = (1 << 5),
    Vendor = (2 << 5),
    Reserved = (3 << 5),

    // Recipient
    Device = (0 << 0),
    Interface = (1 << 0),
    Endpoint = (2 << 0),
    Other = (3 << 0)
};

MAKE_ENUM_BITWISE_OPS(RequestType)

enum Request : u8 {
    GetStatus = 0,
    ClearFeature = 1,
    SetFeature = 3,
    SetAddress = 5,
    GetDescriptor = 6,
    SetDescriptor = 7,
    GetConfiguration = 8,
    SetConfiguration = 9,
    GetInterface = 10,
    SetInterface = 11,
    SynchFrame = 12
};

struct DeviceRequest {
    u8 request_type;
    u8 request;
    u16 value;
    u16 index;
    u16 length;
};

struct DeviceDescriptor {
    u8 length;
    u8 type;
    u16 usb_version;
    u8 device_class;
    u8 device_subclass;
    u8 device_protocol;
    u8 max_packet_size;
    u16 vendor_id;
    u16 product_id;
    u16 device_release;
    u8 manufacturer_index;
    u8 product_index;
    u8 serial_number_index;
    u8 num_configurations;
};

struct UnicodeStringDescriptor {
    u8 length;
    u8 type;
    u16 string[0];
};

}