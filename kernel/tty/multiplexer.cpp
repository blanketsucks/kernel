#include <kernel/tty/multiplexer.h>
#include <kernel/tty/master.h>
#include <kernel/fs/fd.h>

namespace kernel {

static constexpr u32 MAX_PTY_PAIRS = 10;
static PTYMultiplexer* s_instance = nullptr;

PTYMultiplexer::PTYMultiplexer() : CharacterDevice(DeviceMajor::PTYMultiplexer, 0) {
    s_instance = this;
    for (u32 i = 0; i < MAX_PTY_PAIRS; i++) {
        m_free_master_pts.append(i);
    }
}

PTYMultiplexer* PTYMultiplexer::instance() {
    return s_instance;
}

void PTYMultiplexer::add_master_pts(u32 pts) {
    m_free_master_pts.append(pts);
}

RefPtr<fs::FileDescriptor> PTYMultiplexer::open(int options) {
    if (m_free_master_pts.empty()) {
        return nullptr;
    }

    u32 pts = m_free_master_pts.take_last();
    auto master = RefPtr<PTYMaster>::make(pts);

    return fs::FileDescriptor::create(move(master), options);
}

}