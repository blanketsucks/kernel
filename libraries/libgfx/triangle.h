#pragma once

#include <libgfx/point.h>
#include <libgfx/render_context.h>

namespace gfx {

class Triangle {
public:
    Triangle(Point a, Point b, Point c) : m_a(a), m_b(b), m_c(c) {}

    Point a() const { return m_a; }
    Point b() const { return m_b; }
    Point c() const { return m_c; }

    void draw(RenderContext&, u32 color, bool fill = false);

private:
    Point m_a;
    Point m_b;
    Point m_c;
};

}