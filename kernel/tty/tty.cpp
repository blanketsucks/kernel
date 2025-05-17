#include <kernel/tty/tty.h>

namespace kernel {

TTY::TTY(DeviceMajor major, u32 minor) : CharacterDevice(major, minor) {}

ErrorOr<size_t> TTY::read(void* buff, size_t size, size_t) {
    u8* buffer = reinterpret_cast<u8*>(buff);
    size = std::min(size, m_input_buffer.size());

    for (size_t i = 0; i < size; i++) {
        buffer[i] = m_input_buffer.pop();
    }

    return size;
}

ErrorOr<size_t> TTY::write(const void* buffer, size_t size, size_t) {
    this->on_write(static_cast<const u8*>(buffer), size);
    return size;
}


void TTY::emit(u8 byte) {
    m_input_buffer.push(byte);
    this->echo(byte);
}

}