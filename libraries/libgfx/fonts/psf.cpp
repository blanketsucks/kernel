#include <libgfx/fonts/psf.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

namespace gfx {

RefPtr<PSFFont> PSFFont::create(const u8* data) {
    return RefPtr<PSFFont>(new PSFFont(data));
}

RefPtr<PSFFont> PSFFont::create(StringView path) {
    int fd = open_length(path.data(), path.size(), O_RDONLY, 0);
    if (fd < 0) {
        return nullptr;
    }
    
    struct stat st;
    fstat(fd, &st);

    u8* data = new u8[st.st_size];
    read(fd, data, st.st_size);

    close(fd);
    auto font = RefPtr<PSFFont>(new PSFFont(data));

    delete[] data;
    return font;
}

PSFFont::PSFFont(const u8* data) {
    if (*reinterpret_cast<const u16*>(data) == 0x0436) {
        this->parse_psf1(data);
    } else if (*reinterpret_cast<const u32*>(data) == 0x864ab572) {
        this->parse_psf2(data);
    }
}

void PSFFont::parse_psf1(const u8* data) {
    auto* header = reinterpret_cast<const PSFHeader*>(data);
    if (header->mode & (1 << 1)) {
        m_glyph_count = 512;
    } else {
        m_glyph_count = 256;
    }

    m_glyph_size = header->charsize;
    m_width = 8;
    m_height = header->charsize;

    m_data = new u8[m_glyph_count * m_glyph_size];
    std::memcpy(m_data, data + sizeof(PSFHeader), m_glyph_count * m_glyph_size);
}

void PSFFont::parse_psf2(const u8* data) {
    auto* header = reinterpret_cast<const PSF2Header*>(data);

    m_glyph_count = header->glyph_count;
    m_glyph_size = header->glyph_size;
    m_width = header->width;
    m_height = header->height;

    m_data = new u8[m_glyph_count * m_glyph_size];
    if (header->flags & (1 << 0)) {
        // TODO: Parse unicode table
    }

    std::memcpy(m_data, data + header->headersize, m_glyph_count * m_glyph_size);
}

}