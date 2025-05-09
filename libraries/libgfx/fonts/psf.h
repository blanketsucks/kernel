#pragma once

#include <std/memory.h>
#include <std/string_view.h>

namespace gfx {

class PSFFont {
public:
    PSFFont() = default;
    ~PSFFont() = default;

    static RefPtr<PSFFont> create(const u8* data);
    static RefPtr<PSFFont> create(StringView path);

    u8* glyph_data() const { return m_data; }

    size_t glyph_count() const { return m_glyph_count; }
    size_t glyph_size() const { return m_glyph_size; }

    size_t width() const { return m_width; }
    size_t height() const { return m_height; }

private:
    PSFFont(const u8* data);

    void parse_psf1(const u8* data);
    void parse_psf2(const u8* data);

    struct PSFHeader {
        u16 magic;
        u8 mode;
        u8 charsize;
    };

    struct PSF2Header {
        u32 magic;
        u32 version;
        u32 headersize;
        u32 flags;
        u32 glyph_count;
        u32 glyph_size;
        u32 width;
        u32 height;
    };

    size_t m_glyph_count = 0;
    size_t m_glyph_size = 0;
    size_t m_width = 0;
    size_t m_height = 0;

    u8* m_data = nullptr;
};

}