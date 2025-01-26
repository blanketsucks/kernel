#include <libgfx/rect.h>
#include <libgfx/point.h>
#include <std/format.h>

namespace gfx {

void Rect::draw(RenderContext& context, u32 color, bool fill) const {
    if (!fill) {
        Line ab { { x(), y() }, { right(), y() } };
        ab.draw(context, color);

        Line bc { { right(), y() }, { right(), bottom() } };
        bc.draw(context, color);

        Line cd { { right(), bottom() }, { x(), bottom() } };
        cd.draw(context, color);

        Line da { { x(), bottom() }, { x(), y() } };
        da.draw(context, color);

        return;
    }

    auto& framebuffer = context.framebuffer();
    for (int y = top(); y < bottom(); y++) {
        for (int x = left(); x < right(); x++) {
            framebuffer.set_pixel(x, y, color);
        }
    }
}

}