#pragma once

#include <libgfx/ssfn.h>
#include <libgfx/render_context.h>
#include <libgfx/rect.h>
#include <libgfx/point.h>

#include <std/types.h>
#include <std/memory.h>
#include <std/string_view.h>

namespace gfx {

struct BoundingBox {
    int x;
    int y;
    int width;
    int height;

    Rect rect() const {
        return Rect(x, y, width, height);
    }
};

class Font {
public:
    static RefPtr<Font> create(RenderContext&, const u8* data);
    static RefPtr<Font> create(RenderContext&, StringView path);

    u32 width() const { return m_font->width; }
    u32 height() const { return m_font->height; }
    u32 baseline() const { return m_font->baseline; }
    
    int select(RenderContext&, int size) const;
    
    int render(RenderContext&, Point, u32 color, StringView text, int size);
    int render(RenderContext&, Point, u32 color, StringView text, int size, int& advance);

    BoundingBox measure(RenderContext&, StringView text, int size) const;

private:
    Font(RenderContext&, const u8* data);

    const ssfn_font_t* m_font;
};

}