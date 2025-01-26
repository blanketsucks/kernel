#pragma once

#include <libgfx/ssfn.h>
#include <libgfx/render_context.h>
#include <libgfx/rect.h>
#include <libgfx/point.h>

#include <string.h>
#include <std/types.h>
#include <std/memory.h>
#include <std/string.h>
#include <std/string_view.h>

namespace gfx {

class FontContext {
public:
    FontContext() {
        memset(&m_ctx, 0, sizeof(ssfn_t));
    }

    ~FontContext() {
        ssfn_free(&m_ctx);
    }

    ssfn_t* ssfn_ctx() { return &m_ctx; }

    FontContext(const FontContext&) = delete;
    FontContext& operator=(const FontContext&) = delete;

private:
    ssfn_t m_ctx;
};

struct BoundingBox {
    int x;
    int y;
    int width;
    int height;

    Rect rect() const {
        return Rect(x, y, width, height);
    }
};

enum class FontStyle {
    Regular = 0,
    Bold = 1,
    Italic = 2,
    Underline = 4,
};

class Font {
public:
    static RefPtr<Font> create(FontContext&, const u8* data);
    static RefPtr<Font> create(FontContext&, StringView path);

    String name() const { return m_name; }
    String family() const { return m_family; }

    u32 width() const { return m_font->width; }
    u32 height() const { return m_font->height; }
    u32 baseline() const { return m_font->baseline; }
    
    int select(int size) const;
    
    int render(RenderContext&, Point, u32 color, StringView text, int size);
    int render(RenderContext&, Point, u32 color, StringView text, int size, int& advance);

    BoundingBox measure(StringView text, int size) const;

private:
    Font(FontContext&, const u8* data);

    String m_name;
    String m_family;

    FontContext& m_ctx;
    const ssfn_font_t* m_font;
};

}