#pragma once

#include <kernel/common.h>
#include <kernel/devices/character.h>
#include <kernel/cpu/idt.h>
#include <std/vector.h>

namespace kernel::devices {

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
};

class KeyboardDevice : public CharacterDevice {
public:
    enum KeyModifiers : u8 {
        None  = 0,
        Shift = 1 << 0,
        Ctrl  = 1 << 1,
        Alt   = 1 << 2
    };

    static void init();
    static KeyboardDevice* instance();

    size_t read(void* buffer, size_t size, size_t offset) override;
    size_t write(const void* buffer, size_t size, size_t offset) override;

private:
    static constexpr u8 MAX_KEY_BUFFER_SIZE = 255;
    static KeyboardDevice* s_instance;

    KeyboardDevice() : CharacterDevice(13, 1) {}

    INTERRUPT static void handle_interrupt(cpu::InterruptFrame*);

    Vector<KeyEvent> m_key_buffer;
    u8 m_key_buffer_offset = 0;
};

}