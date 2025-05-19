#pragma once

#include <kernel/common.h>
#include <kernel/devices/character_device.h>
#include <kernel/arch/irq.h>

#include <std/vector.h>

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

class KeyboardDevice : public CharacterDevice, IRQHandler {
public:
    enum KeyModifiers : u8 {
        None  = 0,
        Shift = 1 << 0,
        Ctrl  = 1 << 1,
        Alt   = 1 << 2
    };

    static KeyboardDevice* create();

    ErrorOr<size_t> read(void* buffer, size_t size, size_t offset) override;
    ErrorOr<size_t> write(const void* buffer, size_t size, size_t offset) override;

    bool is_full() const { return m_key_buffer.size() == MAX_KEY_BUFFER_SIZE; }

private:
    friend class Device;

    static constexpr u8 MAX_KEY_BUFFER_SIZE = 255;

    KeyboardDevice();

    void handle_irq() override;

    Vector<KeyEvent> m_key_buffer;
    u8 m_key_buffer_offset = 0;
};

}