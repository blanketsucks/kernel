#pragma once

#include <kernel/common.h>
#include <kernel/devices/input/device.h>
#include <kernel/devices/input/manager.h>

namespace kernel {

enum KeyModifiers : u8 {
    None  = 0,
    Shift = 1 << 0,
    Ctrl  = 1 << 1,
    Alt   = 1 << 2
};

struct KeyEvent {
    char ascii;
    u8 scancode;
    u8 modifiers;

    bool is_pressed() const { return this->scancode & 0x80; }

    bool operator==(const KeyEvent& other) const {
        return this->ascii == other.ascii && this->scancode == other.scancode && this->modifiers == other.modifiers;
    }
};

class KeyboardDevice : public InputDevice {

protected:
    KeyboardDevice() : InputDevice(DeviceMajor::Keyboard, InputManager::generate_keyboard_minor()) {}
};

}