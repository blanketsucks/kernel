#pragma once

#include <kernel/devices/character_device.h>
#include <std/string.h>
#include <std/vector.h>

namespace kernel {

class PTYSlave;

class PTYMaster : public devices::CharacterDevice {
public:
    PTYMaster(u32 pts);
    ~PTYMaster();

    ssize_t read(void* buffer, size_t size, size_t offset) override;
    ssize_t write(const void* buffer, size_t size, size_t offset) override;

    u32 pts() const { return m_pts; }
    PTYSlave* slave() const { return m_slave; }

    void on_slave_write(const u8* buffer, size_t size);

private:
    u32 m_pts;

    Vector<u8> m_buffer; // FIXME: Use something different than a Vector
    PTYSlave* m_slave;
};

}
