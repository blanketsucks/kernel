#pragma once

#include <kernel/devices/character_device.h>
#include <std/vector.h>

namespace kernel {

class PTYMultiplexer : public CharacterDevice {
public:
    PTYMultiplexer();

    static PTYMultiplexer* instance();

    RefPtr<fs::FileDescriptor> open(int options) override;

    ssize_t read(void*, size_t, size_t) override { return 0; }
    ssize_t write(const void*, size_t, size_t) override { return 0; }

    void add_master_pts(u32 pts);

private:
    Vector<u32> m_free_master_pts;
};

}