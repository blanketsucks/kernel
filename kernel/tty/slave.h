#pragma once

#include <kernel/tty/tty.h>

namespace kernel {

class PTYMaster;

class PTYSlave : public TTY {
public:
    PTYSlave(u32 pts, PTYMaster* master);
    ~PTYSlave() override;

    u32 pts() const { return m_pts; }
    PTYMaster* master() const { return m_master; }

    size_t on_master_write(const u8* buffer, size_t size);

protected:
    void on_write(const u8* buffer, size_t size) override;
    void echo(u8 byte) override;

private:
    u32 m_pts;

    PTYMaster* m_master;
};

}