#include <libgfx/font.h>
#include <std/kmalloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

namespace gfx {

Font::Font(RenderContext& context, const u8* data) {
    if (!data) {
        return;
    }

    ssfn_load(context.ssfn_ctx(), data, &m_font);
}

RefPtr<Font> Font::create(RenderContext& context, const u8* data) {
    auto font = RefPtr<Font>(new Font(context, data));
    if (!font->m_font) {
        return nullptr;
    }

    return font;
}

RefPtr<Font> Font::create(RenderContext& context, StringView path) {
    int fd = open(path.data(), O_RDONLY);
    if (fd < 0) {
        return nullptr;
    }

    struct stat st;
    fstat(fd, &st);

    u8* data = new u8[st.st_size];
    read(fd, data, st.st_size);

    close(fd);
    auto font = Font::create(context, data);

    delete[] data;
    if (!font) {
        return nullptr;
    }

    return font;
}

int Font::select(RenderContext& context, int size) const {
    if (size > SSFN_SIZE_MAX) {
        return -1;
    }

    auto* ctx = context.ssfn_ctx();

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

    int rc = this->select(context, size);
    if (rc < 0) {
        return rc;
    }

    rc = 0;
    const char* str = text.data();
    while (text.size() > rc) {
        int ret = ssfn_render(context.ssfn_ctx(), &buf, str + rc);
        if (ret < 0) {
            return ret;
        }

        rc += ret;
    }

    advance = buf.x - point.x();
    return rc;
}

BoundingBox Font::measure(RenderContext& context, StringView text, int size) const {
    BoundingBox box = { 0, 0, 0, 0 };
    int rc = this->select(context, size);

    if (rc < 0) {
        return box;
    }

    ssfn_bbox(context.ssfn_ctx(), text.data(), &box.width, &box.height, &box.x, &box.y);
    return box;
}

}