#pragma once

#include <libgfx/vector2.h>
#include <libgfx/render_context.h>

namespace gfx {

using Point = Vector2;

class Line {
public:
    Line(Point start, Point end) : m_start(start), m_end(end) {}

    Point start() const { return m_start; }
    Point end() const { return m_end; }

    void draw(RenderContext&, u32 color);

private:
    Point m_start;
    Point m_end;
};

}