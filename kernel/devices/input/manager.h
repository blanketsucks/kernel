#pragma once

#include <kernel/devices/input/device.h>

#include <std/memory.h>
#include <std/vector.h>

namespace kernel {

class InputManager {
public:
    static void initialize();
    static InputManager* instance();

    static u32 generate_mouse_minor();
    static u32 generate_keyboard_minor();

private:
    void enumerate();
    
    std::Vector<RefPtr<InputDevice>> m_devices;
};


}