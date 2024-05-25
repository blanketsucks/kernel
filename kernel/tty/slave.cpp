#include <kernel/tty/slave.h>
#include <kernel/tty/master.h>

#include <kernel/fs/ptsfs.h>

namespace kernel {

PTYSlave::PTYSlave(u32 pts, PTYMaster* master) : TTY(169, pts), m_pts(pts), m_master(master) {
    fs::PTSFS::register_pty(pts);
}

size_t PTYSlave::on_master_write(const u8* buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        this->emit(buffer[i]);
    }

    return size;
}

void PTYSlave::on_write(const u8* buffer, size_t size) {
    m_master->on_slave_write(buffer, size);
}

void PTYSlave::echo(u8 byte) {
    m_master->on_slave_write(&byte, 1);
}

}