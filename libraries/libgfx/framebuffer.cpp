#include <libgfx/framebuffer.h>
#include <std/kmalloc.h>
#include <stdlib.h>
#include <string.h>

namespace gfx {

FrameBuffer::FrameBuffer(FrameBufferResolution resolution) : m_width(resolution.width), m_height(resolution.height), m_pitch(resolution.pitch) {
    size_t size = m_width * m_height * 4;
    m_buffer = reinterpret_cast<u32*>(malloc(size));
}

void FrameBuffer::set_pixel(u32 x, u32 y, u32 color) {
    if (x >= m_width || y >= m_height) {
        return;
    }

    m_buffer[y * m_width + x] = color;
}

u32 FrameBuffer::get_pixel(u32 x, u32 y) const {
    if (x >= m_width || y >= m_height) {
        return 0;
    }

    return m_buffer[y * m_width + x];
}

void FrameBuffer::clear(u32 color) {
    memset(m_buffer, color, this->size());
}

}