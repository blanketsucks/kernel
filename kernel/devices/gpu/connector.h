#pragma once

#include <kernel/common.h>

namespace kernel {

class GPUConnector {
public:
    virtual ~GPUConnector() = default;

    u32 id() const { return m_id; }

protected:
    GPUConnector(u32 id);

private:
    u32 m_id;
};

}