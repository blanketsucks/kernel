#pragma once

#include <kernel/common.h>
#include <kernel/devices/input/keyboard.h>
#include <kernel/arch/irq.h>

#include <std/vector.h>

namespace kernel {

class PS2KeyboardDevice : public KeyboardDevice, IRQHandler {
public:
    enum KeyModifiers : u8 {
        None  = 0,
        Shift = 1 << 0,
        Ctrl  = 1 << 1,
        Alt   = 1 << 2
    };

    static RefPtr<InputDevice> create();

    ErrorOr<size_t> read(void* buffer, size_t size, size_t offset) override;
    ErrorOr<size_t> write(const void* buffer, size_t size, size_t offset) override;

    bool is_full() const { return m_key_buffer.size() == MAX_KEY_BUFFER_SIZE; }

    bool can_read(fs::FileDescriptor const&) const override;
    bool can_write(fs::FileDescriptor const&) const override { return false; }

private:
    friend class Device;

    static constexpr u8 MAX_KEY_BUFFER_SIZE = 255;

    PS2KeyboardDevice();

    void handle_irq() override;

    Vector<KeyEvent> m_key_buffer;
    u8 m_key_buffer_offset = 0;
};

}