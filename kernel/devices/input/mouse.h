#pragma once

#include <kernel/common.h>
#include <kernel/devices/character_device.h>
#include <kernel/arch/irq.h>

#include <std/enums.h>
#include <std/circular_queue.h>

namespace kernel {

constexpr u16 MOUSE_STATUS = 0x64;
constexpr u16 MOUSE_DATA   = 0x60;
constexpr u16 MOUSE_WRITE  = 0xD4;
constexpr u16 MOUSE_A_BIT  = 0x02;
constexpr u16 MOUSE_B_BIT  = 0x01;
constexpr u16 MOUSE_F_BIT  = 0x20;

constexpr u16 MOUSE_SET_SAMPLE_RATE = 0xF3;
constexpr u16 MOUSE_GET_DEVICE_ID   = 0xF2;
constexpr u16 MOUSE_ENABLE_STREAM   = 0xF4;
constexpr u16 MOUSE_SET_DEFAULTS    = 0xF6;

struct MousePacket {
    u8 lb  : 1; // Left Button
    u8 rb  : 1; // Right Button
    u8 mb  : 1; // Middle Button
    u8 one : 1; // Always 1
    u8 xs  : 1; // X-Axis Sign Bit (9-Bit X-Axis Relative Offset)
    u8 ys  : 1; // Y-Axis Sign Bit (9-Bit Y-Axis Relative Offset)
    u8 xo  : 1; // X-Axis Overflow
    u8 yo  : 1; // Y-Axis Overflow

    u8 x;       // X-Axis Movement Value
    u8 y;       // Y-Axis Movement Value
    u8 z;       // Z-Axis Movement Value
};

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

class MouseDevice : public CharacterDevice, IRQHandler {
public:
    static void init();
    static MouseDevice* instance();

    ErrorOr<size_t> read(void* buffer, size_t size, size_t offset) override;
    ErrorOr<size_t> write(const void* buffer, size_t size, size_t offset) override;

    bool has_scroll_wheel() const { return m_has_scroll_wheel; }

    u8 read();
    void wait(u8 type);
    void write(u8 data);

    void set_sample_rate(u8 rate);
    u8 get_device_id();

    MouseState state();

private:
    static constexpr u8 MAX_BUFFER_SIZE = 255;
    static MouseDevice* s_instance;

    void update_mouse_state();
    void handle_irq() override;

    MouseDevice() : CharacterDevice(DeviceMajor::Input, 0), IRQHandler(12) {}

    CircularQueue<MouseState, MAX_BUFFER_SIZE> m_state_buffer;
    u8 m_cycle;
    u8 m_bytes[4];

    bool m_has_scroll_wheel;
};

}