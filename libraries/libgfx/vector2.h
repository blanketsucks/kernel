#pragma once

#include <std/types.h>

namespace gfx {

class Vector2 {
public:
    Vector2() = default;
    Vector2(int x, int y) : m_x(x), m_y(y) {}

    int x() const { return m_x; }
    int y() const { return m_y; }

    Vector2 translate(int dx, int dy) const {
        return Vector2(m_x + dx, m_y + dy);
    }

    Vector2 translate(const Vector2& other) const {
        return translate(other.x(), other.y());
    }

private:
    int m_x;
    int m_y;
};

}