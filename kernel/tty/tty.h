#pragma once

#include <kernel/devices/character_device.h>

#include <std/circular_queue.h>

namespace kernel {

class TTY : public CharacterDevice {
public:
    virtual ~TTY() = default;

    ErrorOr<size_t> read(void* buffer, size_t size, size_t offset) override;
    ErrorOr<size_t> write(const void* buffer, size_t size, size_t offset) override;

    void emit(u8 byte);

protected:
    TTY(DeviceMajor major, u32 minor);

    virtual void on_write(const u8* buffer, size_t size) = 0;
    virtual void echo(u8) = 0;

private:
    std::CircularQueue<u8, 1024> m_input_buffer;
};

}