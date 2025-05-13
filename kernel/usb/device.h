#pragma once

#include <kernel/common.h>
#include <kernel/usb/pipe.h>

#include <std/memory.h>

namespace kernel::usb {

class Controller;

class Device {
public:
    static RefPtr<Device> create(Controller*, u8 port, u8 address = 0);
    void initialize();
    
    Controller* controller() const { return m_controller; }
    
    u8 port() const { return m_port; }
    u8 address() const { return m_address; }
    
private:
    Device(Controller* controller, u8 port, u8 address) : m_controller(controller), m_port(port), m_address(address) {}

    void set_device_address(u8 address);

    Controller* m_controller;

    u8 m_port;
    u8 m_address;

    OwnPtr<ControlPipe> m_default_control_pipe;
};

}