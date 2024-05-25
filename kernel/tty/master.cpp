#include <kernel/tty/master.h>
#include <kernel/tty/slave.h>
#include <kernel/tty/multiplexer.h>

namespace kernel {

PTYMaster::PTYMaster(u32 pts) : devices::CharacterDevice(168, pts), m_pts(pts) {
    m_slave = new PTYSlave(pts, this);
}

PTYMaster::~PTYMaster() {
    delete m_slave;
    PTYMultiplexer::instance()->add_master_pts(m_pts);
}

ssize_t PTYMaster::read(void* buff, size_t size, size_t) {
    u8* buffer = reinterpret_cast<u8*>(buff);
    size = std::min(size, m_buffer.size());

    for (size_t i = 0; i < size; i++) {
        buffer[i] = m_buffer.take_first();
    }

    return size;
}

ssize_t PTYMaster::write(const void* buffer, size_t size, size_t) {
    return m_slave->on_master_write(reinterpret_cast<const u8*>(buffer), size);
}

void PTYMaster::on_slave_write(const u8* buffer, size_t size) {
    m_buffer.append(buffer, size);
}


}