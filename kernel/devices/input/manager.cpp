#include <kernel/devices/input/manager.h>
#include <kernel/fs/devfs/filesystem.h>

#include <kernel/devices/input/ps2/keyboard.h>
#include <kernel/devices/input/ps2/mouse.h>

namespace kernel {

static InputManager s_instance;
static u32 s_mouse_minor = 0;
static u32 s_keyboard_minor = 0;

InputManager* InputManager::instance() {
    return &s_instance;
}

u32 InputManager::generate_mouse_minor() {
    return s_mouse_minor++;
}

u32 InputManager::generate_keyboard_minor() {
    return s_keyboard_minor++;
}

void InputManager::initialize() {
    auto& subsystem = devfs::create_subsystem("input");
    
    subsystem.add_range("mouse", DeviceMajor::Mouse);
    subsystem.add_range("keyboard", DeviceMajor::Keyboard);

    s_instance.enumerate();
}

void InputManager::enumerate() {
    auto mouse = PS2MouseDevice::create();
    auto keyboard = PS2KeyboardDevice::create();

    m_devices.append(mouse);
    m_devices.append(keyboard);
}

}