#pragma once

#include <std/types.h>
#include <sys/ioctl.h>

namespace gfx {

class FrameBuffer {
public:
    FrameBuffer(
        u32* data, FrameBufferResolution resolution
    ) : m_buffer(data), m_width(resolution.width), m_height(resolution.height), m_pitch(resolution.pitch) {}
    
    FrameBuffer(FrameBufferResolution resolution);

    u32 width() const { return m_width; }
    u32 height() const { return m_height; }
    u32 pitch() const { return m_pitch; }

    size_t size() const { return m_width * m_height * 4; }

    u32* buffer() { return m_buffer; }
    const u32* buffer() const { return m_buffer; }
    
    void set_pixel(u32 x, u32 y, u32 color);
    u32 get_pixel(u32 x, u32 y) const;

    void clear(u32 color);
    
private:
    u32* m_buffer;
    
    u32 m_width;
    u32 m_height;
    u32 m_pitch;
};

}