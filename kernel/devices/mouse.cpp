#include <kernel/devices/mouse.h>
#include <kernel/io.h>
#include <kernel/serial.h>

#include <kernel/cpu/idt.h>
#include <kernel/cpu/pic.h>

#include <std/string.h>
#include <std/utility.h>

namespace kernel::devices {

MouseDevice* MouseDevice::s_instance = nullptr;

void MouseDevice::update_mouse_state() {
    m_cycle = 0;
    MousePacket* packet = reinterpret_cast<MousePacket*>(m_bytes);

    if (packet->xo || packet->yo) {
        return; // We discard the whole packet if we detect an overflow
    }

    MouseState state = {};
    if (packet->lb) {
        state.buttons |= MouseButton::Left;
    } if (packet->rb) {
        state.buttons |= MouseButton::Right;
    } if (packet->mb) {
        state.buttons |= MouseButton::Middle;
    }

    state.x = packet->x - (packet->xs ? 0x100 : 0);
    state.y = packet->y - (packet->ys ? 0x100 : 0);

    if (m_has_scroll_wheel && packet->z) {
        state.z = (packet->z == 0xFF) ? -1 : packet->z; 
    } else {
        state.z = 0;
    }

    m_state = state;
}

void MouseDevice::handle_interrupt(cpu::InterruptFrame*) {
    MouseDevice* device = instance();
    u8 status = io::read<u8>(MOUSE_STATUS);

    while (status & MOUSE_B_BIT) {
        if (!(status & MOUSE_F_BIT)) {
            status = io::read<u8>(MOUSE_STATUS);
            continue;
        }

        u8 packet = io::read<u8>(MOUSE_DATA);
        device->m_bytes[device->m_cycle] = packet;

        switch (device->m_cycle) {
            case 0:
                device->m_cycle++;
                break;
            case 1:
                device->m_cycle++;
                break;
            case 2:
                if (device->m_has_scroll_wheel) {
                    device->m_cycle++;
                    break;
                }
            case 3:
                device->update_mouse_state();
                break;
        }

        status = io::read<u8>(MOUSE_STATUS);
    }

    pic::send_eoi(12);
}

MouseDevice* MouseDevice::instance() {
    return s_instance;
}

void MouseDevice::wait(u8 type) {
    u32 timeout = 100000;
    if (!type) {
        while (timeout--) {
            if ((io::read<u8>(MOUSE_STATUS) & MOUSE_A_BIT) == 1) {
                return;
            }
        }

        return;
    }

    while (timeout--) {
        if ((io::read<u8>(MOUSE_STATUS) & MOUSE_B_BIT) == 0) {
            return;
        }
    }
}

void MouseDevice::write(u8 value) {
    this->wait(1);
    io::write<u8>(MOUSE_STATUS, MOUSE_WRITE);
    this->wait(1);
    io::write<u8>(MOUSE_DATA, value);
}

u8 MouseDevice::read() {
    this->wait(0);
    return io::read<u8>(MOUSE_DATA);
}

void MouseDevice::init() {
    // Enable the auxiliary mouse device
    MouseDevice* device = new MouseDevice();
    s_instance = device;

    device->wait(1);
    io::write<u8>(MOUSE_STATUS, 0xA8);

    // Enable interrupts
    device->wait(1);
    io::write<u8>(MOUSE_STATUS, 0x20);
    device->wait(0);
    u8 status = io::read<u8>(MOUSE_DATA) | 2;
    device->wait(1);
    io::write<u8>(MOUSE_STATUS, 0x60);
    device->wait(1);
    io::write<u8>(MOUSE_DATA, status);

    // Use the default settings and enable
    device->write(MOUSE_SET_DEFAULTS); device->read();
    device->write(MOUSE_ENABLE_STREAM); device->read();

    u8 device_id = device->get_device_id();
    if (device_id != 0x03) {
        // Enable Z-Axis (scroll wheel)
        device->set_sample_rate(200);
        device->set_sample_rate(100);
        device->set_sample_rate(80);

        device_id = device->get_device_id();
    }

    if (device_id == 0x03) {
        device->m_has_scroll_wheel = true;
    }

    cpu::set_idt_entry(0x2C, reinterpret_cast<u32>(MouseDevice::handle_interrupt), 0x8E);
    pic::enable(12);
}

u8 MouseDevice::get_device_id() {
    this->write(MOUSE_GET_DEVICE_ID); this->read();
    return this->read();
}

void MouseDevice::set_sample_rate(u8 rate) {
    this->write(MOUSE_SET_SAMPLE_RATE); this->read();
    this->write(rate); this->read();
}

MouseState MouseDevice::state() { return m_state; }

size_t MouseDevice::read(void*, size_t, size_t) {
    return 0;
}

size_t MouseDevice::write(const void*, size_t, size_t) {
    return 0;
}

}