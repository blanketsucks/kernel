#pragma once

#include <kernel/common.h>
#include <kernel/process/process.h>

#include <std/result.h>

namespace kernel {

class GPUConnector {
public:
    struct Resolution {
        int width;
        int height;
        int pitch;
        int bpp;
    };

    virtual ~GPUConnector() = default;

    u32 id() const { return m_id; }

    virtual ErrorOr<Resolution> get_resolution() const = 0;
    virtual ErrorOr<void*> map_framebuffer(Process*) = 0;
    virtual ErrorOr<void> flush() = 0;

protected:
    GPUConnector(u32 id) : m_id(id) {}

    u32 m_id;
};

}