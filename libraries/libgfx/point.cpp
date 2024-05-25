#include <libgfx/point.h>
#include <std/utility.h>
#include <std/format.h>

namespace gfx {

void Line::draw(RenderContext& context, u32 color) {
    Point start = m_start;
    Point end = m_end;

    i32 dx = end.x() - start.x();
    i32 dy = end.y() - start.y();

    auto& framebuffer = context.framebuffer();

    if (dx == 0 && dy != 0) {        // Vertical line
        if (start.y() > end.y()) {
            std::swap(start, end);
        }

        for (i32 y = start.y(); y <= end.y(); y++) {
            framebuffer.set_pixel(start.x(), y, color);
        }

        return;
    } else if (dx != 0 && dy == 0) { // Horizontal line
        if (start.x() > end.x()) {
            std::swap(start, end);
        }

        for (i32 x = start.x(); x <= end.x(); x++) {
            framebuffer.set_pixel(x, start.y(), color);
        }

        return;
    }

    if (!dx && !dy) {
        framebuffer.set_pixel(start.x(), start.y(), color);
        return;
    }

    if (std::abs(dx) > std::abs(dy)) {
        if (start.x() > end.x()) {
            std::swap(start, end);
        }

        for (i32 x = start.x(); x <= end.x(); x++) {
            i32 y = start.y() + dy * (x - start.x()) / dx;
            framebuffer.set_pixel(x, y, color);
        }
    } else {
        if (start.y() > end.y()) {
            std::swap(start, end);
        }

        for (i32 y = start.y(); y <= end.y(); y++) {
            i32 x = start.x() + dx * (y - start.y()) / dy;
            framebuffer.set_pixel(x, y, color);
        }
    }
}

}