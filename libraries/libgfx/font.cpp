#include <libgfx/font.h>
#include <std/kmalloc.h>
#include <std/format.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

namespace gfx {

Font::Font(FontContext& ctx, const u8* data) : m_ctx(ctx) {
    if (!data) {
        return;
    }

    ssfn_load(ctx.ssfn_ctx(), data, &m_font);

    const char* string_table = reinterpret_cast<const char*>(data + sizeof(ssfn_font_t));
    
    m_name = String(string_table);
    m_family = String(string_table + m_name.size() + 1);
}

RefPtr<Font> Font::create(FontContext& ctx, const u8* data) {
    auto font = RefPtr<Font>(new Font(ctx, data));
    if (!font->m_font) {
        return nullptr;
    }

    return font;
}

RefPtr<Font> Font::create(FontContext& ctx, StringView path) {
    int fd = open(path.data(), O_RDONLY);
    if (fd < 0) {
        return nullptr;
    }

    struct stat st;
    fstat(fd, &st);

    u8* data = new u8[st.st_size];
    read(fd, data, st.st_size);

    close(fd);
    auto font = Font::create(ctx, data);

    if (!font) {
        return nullptr;
    }

    return font;
}

int Font::select(int size) const {
    auto* ctx = m_ctx.ssfn_ctx();
    if (size > SSFN_SIZE_MAX) {
        return -1;
    } else if (ctx->s == m_font && ctx->size == size) {
        return SSFN_OK;
    }

    ctx->s = m_font;
    ctx->family = SSFN_TYPE_FAMILY(m_font->type);
    ctx->style = SSFN_TYPE_STYLE(m_font->type);
    ctx->size = size;

    return SSFN_OK;
}

int Font::render(RenderContext& context, Point point, u32 color, StringView text, int size) {
    int advance = 0;
    return this->render(context, point, color, text, size, advance);
}

int Font::render(RenderContext& context, Point point, u32 color, StringView text, int size, int& advance) {
    ssfn_buf_t buf;
    auto& framebuffer = context.framebuffer();

    buf.ptr = reinterpret_cast<u8*>(framebuffer.buffer());
    buf.w = framebuffer.width();
    buf.h = framebuffer.height();
    buf.p = framebuffer.pitch();
    buf.x = point.x();
    buf.y = point.y();
    buf.fg = color;
    buf.bg = 0;

    int rc = this->select(size);
    if (rc < 0) {
        return rc;
    }

    rc = 0;
    const char* str = text.data();
    while (text.size() > rc) {
        int ret = ssfn_render(m_ctx.ssfn_ctx(), &buf, str + rc);
        if (ret < 0) {
            return ret;
        }

        rc += ret;
    }

    advance = buf.x - point.x();
    return rc;
}

BoundingBox Font::measure(StringView text, int size) const {
    BoundingBox box = { 0, 0, 0, 0 };
    int rc = this->select(size);

    if (rc < 0) {
        return box;
    }

    ssfn_bbox(m_ctx.ssfn_ctx(), text.data(), &box.width, &box.height, &box.x, &box.y);
    return box;
}

}