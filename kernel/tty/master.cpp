#include <kernel/tty/master.h>
#include <kernel/tty/slave.h>
#include <kernel/tty/multiplexer.h>
#include <kernel/posix/sys/ioctl.h>
#include <kernel/process/process.h>
#include <kernel/process/scheduler.h>

namespace kernel {

PTYMaster::PTYMaster(u32 pts) : CharacterDevice(DeviceMajor::MasterPTY, pts), m_pts(pts) {
    m_slave = Device::create<PTYSlave>(pts, this).take();
}

PTYMaster::~PTYMaster() {
    // TODO: Remove the slave from the global list of devices
    delete m_slave;
    m_slave = nullptr;

    PTYMultiplexer::instance()->add_master_pts(m_pts);
}

ErrorOr<size_t> PTYMaster::read(void* buff, size_t size, size_t) {
    if (!m_slave) {
        return Error(ENODEV);
    }

    u8* buffer = reinterpret_cast<u8*>(buff);
    size = std::min(size, m_buffer.size());

    for (size_t i = 0; i < size; i++) {
        buffer[i] = m_buffer.take_first();
    }

    return size;
}

void PTYMaster::close() {}

ErrorOr<size_t> PTYMaster::write(const void* buffer, size_t size, size_t) {
    if (!m_slave) {
        return Error(ENODEV);
    }

    return m_slave->on_master_write(reinterpret_cast<const u8*>(buffer), size);
}

void PTYMaster::on_slave_write(const u8* buffer, size_t size) {
    m_buffer.append(buffer, size);
}

ErrorOr<int> PTYMaster::ioctl(unsigned request, unsigned arg) {
    Process* process = Process::current();
    switch (request) {
        case TIOCGPTN: {
            process->validate_write(reinterpret_cast<int*>(arg), sizeof(int));
            *reinterpret_cast<int*>(arg) = m_pts;

            return 0;
        }
        default:
            return Error(EINVAL);
    }
}


}