#pragma once

#include <kernel/common.h>

#include <kernel/usb/usb.h>

namespace kernel::usb {

class Pipe;

class Controller {
public:
    virtual ~Controller() = default;

    virtual u8 allocate_device_address() = 0;

    virtual size_t submit_control_transfer(Pipe*, const DeviceRequest& request, PhysicalAddress buffer, size_t length) = 0;
};

}