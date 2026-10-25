#pragma once

#include <kernel/common.h>
#include <kernel/devices/input/device.h>
#include <kernel/devices/input/manager.h>

#include <sys/dev/keyboard.h>

namespace kernel {

class KeyboardDevice : public InputDevice {

protected:
    KeyboardDevice() : InputDevice(DeviceMajor::Keyboard, InputManager::generate_keyboard_minor()) {}
};

}