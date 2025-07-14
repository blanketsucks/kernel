#pragma once

#include <libgfx/framebuffer.h>

namespace gfx {

class RenderContext {
public:
    RenderContext(FrameBuffer framebuffer);

    FrameBuffer& framebuffer() { return m_framebuffer; }
    void set_framebuffer(FrameBuffer framebuffer);

private:
    FrameBuffer m_framebuffer;
};

}