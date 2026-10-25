#pragma once

#include <kernel/devices/input/device.h>
#include <kernel/devices/input/manager.h>

#include <sys/dev/mouse.h>

namespace kernel {

class MouseDevice : public InputDevice {

protected:
    MouseDevice() : InputDevice(DeviceMajor::Mouse, InputManager::generate_mouse_minor()) {}
};

}