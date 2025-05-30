#pragma once

#include <std/memory.h>

namespace kernel::usb {

class Device;

class Pipe {
public:
    enum Direction {
        In = 0,
        Out = 1,
        BiDirectional = 2
    };

    enum Type {
        Control = 0,
        Isochronous = 1,
        Bulk = 2,
        Interrupt = 3
    };

    Device* device() const { return m_device; }

    Direction direction() const { return m_direction; }
    Type type() const { return m_type; }

    u8 endpoint() const { return m_endpoint; }

    u8 max_packet_size() const { return m_max_packet_size; }
    void set_max_packet_size(u8 size) { m_max_packet_size = size; }

    bool data_toggle() const { return m_data_toggle; }
    void set_data_toggle(bool toggle) { m_data_toggle = toggle; }

protected:
    Pipe(
        Device* device, Direction direction, Type type, u8 endpoint, u8 max_packet_size
    ) : m_device(device), m_direction(direction), m_type(type), m_endpoint(endpoint), m_max_packet_size(max_packet_size) {}

    Device* m_device;
    Direction m_direction;
    Type m_type;
    u8 m_endpoint;
    u8 m_max_packet_size;

    bool m_data_toggle = false;
};

class ControlPipe : public Pipe {
public:
    static OwnPtr<ControlPipe> create(Device* device, u8 endpoint, u8 max_packet_size);

    void submit_transfer(u8 request_type, u8 request, u16 value, u16 index, u16 length, void* data);

private:
    ControlPipe(Device* device, u8 endpoint, u8 max_packet_size);

    u8* m_buffer;
};

class BulkPipe : public Pipe {
public:
    static OwnPtr<BulkPipe> create(Device* device, Direction direction, u8 endpoint, u8 max_packet_size);

    void submit_transfer(void* data, size_t length);

private:
    BulkPipe(Device* device, Direction direction, u8 endpoint, u8 max_packet_size);

    u8* m_buffer;
};

}