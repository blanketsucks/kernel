#pragma once

#include <kernel/devices/character_device.h>
#include <std/string.h>
#include <std/vector.h>

namespace kernel {

class PTYSlave;

class PTYMaster : public CharacterDevice {
public:
    PTYMaster(u32 pts);
    ~PTYMaster();

    ErrorOr<size_t> read(void* buffer, size_t size, size_t offset) override;
    ErrorOr<size_t> write(const void* buffer, size_t size, size_t offset) override;

    bool can_read(fs::FileDescriptor const&) const override;
    bool can_write(fs::FileDescriptor const&) const override { return true; }

    void close() override;

    ErrorOr<int> ioctl(unsigned, unsigned) override;

    u32 pts() const { return m_pts; }
    PTYSlave* slave() const { return m_slave; }

    void on_slave_write(const u8* buffer, size_t size);

private:
    u32 m_pts;

    Vector<u8> m_buffer; // FIXME: Use something different than a Vector
    PTYSlave* m_slave;
};

}
