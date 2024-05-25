#include <libgfx/render_context.h>
#include <std/kmalloc.h>

namespace gfx {

RenderContext::RenderContext(FrameBuffer framebuffer) : m_framebuffer(framebuffer) {
    m_ssfn_ctx = new ssfn_t;
}

}