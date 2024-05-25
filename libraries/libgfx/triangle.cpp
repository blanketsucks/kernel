#include <libgfx/triangle.h>
#include <libgfx/framebuffer.h>

namespace gfx {

void Triangle::draw(RenderContext& context, u32 color, bool) {
    Line ab { m_a, m_b };
    ab.draw(context, color);

    Line bc { m_b, m_c };
    bc.draw(context, color);

    Line ca { m_c, m_a };
    ca.draw(context, color);
}

}

