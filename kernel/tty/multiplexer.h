#pragma once

#include <kernel/devices/character_device.h>
#include <std/vector.h>

namespace kernel {

class PTYMultiplexer : public CharacterDevice {
public:
    PTYMultiplexer();

    static PTYMultiplexer* instance();
    static PTYMultiplexer* create();

    RefPtr<fs::FileDescriptor> open(int options) override;

    ErrorOr<size_t> read(void*, size_t, size_t) override { return 0; }
    ErrorOr<size_t> write(const void*, size_t, size_t) override { return 0; }

    void add_master_pts(u32 pts);

private:
    Vector<u32> m_free_master_pts;
};

}