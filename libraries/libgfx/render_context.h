#pragma once

#include <libgfx/ssfn.h>

#include <libgfx/framebuffer.h>

namespace gfx {

class RenderContext {
public:
    RenderContext(FrameBuffer framebuffer);

    FrameBuffer& framebuffer() { return m_framebuffer; }
    void set_framebuffer(FrameBuffer framebuffer);

    ssfn_t* ssfn_ctx() { return m_ssfn_ctx; }

private:
    FrameBuffer m_framebuffer;
    ssfn_t* m_ssfn_ctx;
};

}