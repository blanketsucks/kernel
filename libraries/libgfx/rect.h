#pragma once

#include <libgfx/render_context.h>

#include <std/types.h>
#include <std/utility.h>

namespace gfx {

class Rect {
public:
    Rect(int x, int y, int width, int height) : m_x(x), m_y(y), m_width(width), m_height(height) {}

    int x() const { return m_x; }
    int y() const { return m_y; }
    int width() const { return m_width; }
    int height() const { return m_height; }

    int left() const { return m_x; }
    int right() const { return m_x + m_width; }

    int top() const { return m_y; }
    int bottom() const { return m_y + m_height; }

    void set_coordinates(int x, int y) {
        m_x = x;
        m_y = y;
    }

    void set_size(int width, int height) {
        m_width = width;
        m_height = height;
    }

    bool contains(int x, int y) const {
        return x >= left() && x < right() && y >= top() && y < bottom();
    }

    bool intersects(const Rect& other) const {
        return left() < other.right() && right() > other.left() && top() < other.bottom() && bottom() > other.top();
    }

    Rect intersection(const Rect& other) const {
        int x = std::max(left(), other.left());
        int y = std::max(top(), other.top());

        int width = std::min(right(), other.right()) - x;
        int height = std::min(bottom(), other.bottom()) - y;

        return Rect(x, y, width, height);
    }

    Rect scale(int factor) const {
        return Rect(m_x * factor, m_y * factor, m_width * factor, m_height * factor);
    }

    Rect translate(int dx, int dy) const {
        return Rect(m_x + dx, m_y + dy, m_width, m_height);
    }

    Rect move_to(int x, int y) const {
        return Rect(x, y, m_width, m_height);
    }

    Rect resize(int width, int height) const {
        return Rect(m_x, m_y, width, height);
    }

    void draw(RenderContext&, u32 color, bool fill = true) const;

private:
    int m_x;
    int m_y;
    int m_width;
    int m_height;
};

}