#pragma once

#include <kernel/devices/input/device.h>
#include <kernel/devices/input/manager.h>

namespace kernel {

enum class MouseButton : u8 {
    Left   = 1 << 0,
    Right  = 1 << 1,
    Middle = 1 << 2
};

MAKE_ENUM_BITWISE_OPS(MouseButton);

struct MouseState {
    i32 x, y, z;
    MouseButton buttons;

    bool is_pressed(MouseButton button) const { return std::has_flag(buttons, button); }

    bool operator==(MouseState const& other) const {
        return this->x == other.x && this->y == other.y && this->z == other.z && this->buttons == other.buttons;
    }
};

class MouseDevice : public InputDevice {

protected:
    MouseDevice() : InputDevice(DeviceMajor::Mouse, InputManager::generate_mouse_minor()) {}
};

}