#include <kernel/devices/keyboard.h>
#include <kernel/vga.h>
#include <kernel/io.h>

#include <std/string.h>
#include <std/format.h>

namespace kernel::devices {

KeyboardDevice* KeyboardDevice::s_instance = nullptr;

static const char SCANCODE_MAP[0x80] = {
    0, '\033', '1', '2', '3', '4',
    '5', '6', '7', '8', '9', '0',
    '-', '=', 0x08, '\t', 'q', 'w','e',
    'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']', '\n', 0, 'a', 's', 'd',
    'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'', '`', 0, '\\', 'z', 'x', 'c',
    'v', 'b', 'n', 'm', ',', '.', '/',
    0, '*', 0, ' ', 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, '-', 0, 0, 0, '+', 0,
    0, 0, 0, 0, 0, 0, '\\', 0,0, 0,
};

static const char SHIFT_SCANCODE_MAP[0x80] = {
    0, '\033', '!', '@', '#', '$', '%',
    '^', '&', '*', '(', ')', '_', '+',
    0x08, '\t', 'Q', 'W', 'E', 'R', 'T',
    'Y', 'U', 'I', 'O', 'P', '{', '}',
    '\n', 0, 'A', 'S', 'D', 'F', 'G', 'H', 
    'J', 'K', 'L', ':', '"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<',
    '>', '?', 0, '*', 0, ' ', 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, '-', 0, 0, 0, '+', 0, 0,
    0, 0, 0, 0, 0, '|', 0, 0, 0,
};

static bool s_shift = false;
static bool s_ctrl  = false;
static bool s_alt   = false;

void KeyboardDevice::handle_interrupt(arch::InterruptRegisters*) {
    pic::eoi(1);

    u8 scancode = io::read<u8>(0x60);
    u8 modifiers = None;

    if (scancode == 0x2A || scancode == 0x36) {
        s_shift = true;
    } else if (scancode == 0xAA || scancode == 0xB6) {
        s_shift = false;
    } else if (scancode == 0x1D) {
        s_ctrl = true;
    } else if (scancode == 0x9D) {
        s_ctrl = false;
    } else if (scancode == 0x38) {
        s_alt = true;
    } else if (scancode == 0xB8) {
        s_alt = false;
    }

    if (s_shift) modifiers |= Shift;
    if (s_ctrl)  modifiers |= Ctrl;
    if (s_alt)   modifiers |= Alt;

    char ascii = 0;
    if (s_shift) {
        ascii = SHIFT_SCANCODE_MAP[scancode & 0x7F];
    } else {
        ascii = SCANCODE_MAP[scancode & 0x7F];
    }

    KeyEvent event = {
        .ascii = ascii,
        .scancode = scancode,
        .modifiers = modifiers
    };

    if (this->is_full()) {
        m_key_buffer.remove(0);
    }

    m_key_buffer.append(event);
}

ssize_t KeyboardDevice::read(void* buffer, size_t size, size_t) {
    size_t i = 0;
    while (i < size) {
        if (m_key_buffer.empty()) {
            break;
        }

        auto key = m_key_buffer.take_first();
        memcpy(reinterpret_cast<u8*>(buffer) + i, &key, sizeof(KeyEvent));

        i += sizeof(KeyEvent);   
    }

    return i;
}

ssize_t KeyboardDevice::write(const void*, size_t, size_t) {
    return 0;
}

void KeyboardDevice::init() {
    auto* device = new KeyboardDevice();
    device->m_key_buffer.reserve(MAX_KEY_BUFFER_SIZE);

    s_instance = device;
}

KeyboardDevice* KeyboardDevice::instance() {
    return s_instance;
}

}