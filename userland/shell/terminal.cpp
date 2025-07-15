#include <shell/terminal.h>
#include <std/format.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

namespace shell {

void _free(void* ptr, size_t) {
    free(ptr);
}

Terminal::Terminal(
    gfx::RenderContext& context, RefPtr<gfx::PSFFont> font, bool use_flanterm
) : on_line_flush(nullptr), m_render_context(context), m_font(move(font)) {
    auto& fb = m_render_context.framebuffer();
    
    m_height = fb.height();
    m_width = fb.width();
    m_pitch = fb.pitch();

    m_rows = m_height / m_font->height();
    m_cols = m_width / m_font->width();

    if (use_flanterm) {
        m_flanterm_context = flanterm_fb_init(
            malloc, _free,
            fb.buffer(), fb.width(), fb.height(), fb.pitch(),
            8, 16,
            8, 8,
            8, 0,
            nullptr,
            nullptr, nullptr,
            nullptr, nullptr,
            nullptr, nullptr,
            m_font->glyph_data(), m_font->width(), m_font->height(), 1,
            0, 0,
            0
        );
    }

    this->add_line({});
}

String Terminal::cwd() {
    char buffer[256];
    getcwd(buffer, sizeof(buffer));

    return String(buffer);
}

Line& Terminal::current_line() { return m_lines[m_current_line]; }
Line const& Terminal::current_line() const { return m_lines[m_current_line]; }

void Terminal::add_line(String text, bool newline_before) {
    String line = format("{} $ {}", cwd(), text);
    size_t prompt_length = line.size() - text.size();

    if (newline_before) {
        this->render('\n');
    }

    this->render(line);
    m_lines.append({ move(line), prompt_length });
}

void Terminal::write(StringView text) {
    this->render(text);
}

void Terminal::writeln(StringView text) {
    write(text);
    this->render('\n');
}

void Terminal::on_char(char c) {
    auto& line = current_line();
    if (c == '\n') {
        this->render('\n');

        this->on_line_flush(line.text.substr(line.prompt));
        this->add_line({});

        m_current_line++;
        return;
    } else if (c == '\b') {
        if (line.text.size() == line.prompt) {
            return;
        }

        line.text.pop();
        this->render("\b \b");
    } else {
        line.text.append(c);
        this->render(c);
    }
}

void Terminal::clear() {
    m_lines.clear();
    this->add_line({});
    
    m_current_line = 0;
}

void Terminal::replace_current_line(const String& text) {
    auto& line = this->current_line();
    if (m_flanterm_context) {
        for (size_t i = 0; i < line.text.size(); i++) {
            flanterm_write(m_flanterm_context, "\b \b", 3);
        }
    } else {
        for (size_t i = 0; i < line.text.size(); i++) {
            this->render("\b \b");
        }
    }

    line.text = format("{} $ {}", cwd(), text);
    line.prompt = line.text.size() - text.size();

    this->render(line.text);
}

void Terminal::render(char c) {
    if (m_flanterm_context) {
        flanterm_write(m_flanterm_context, &c, 1);
        return;
    }

    if (c == '\n') {
        this->clear_cursor();

        m_x = 0;
        m_y += m_font->height();

        if (m_y + m_font->height() > m_height) {
            this->scroll();
        }

        this->render_cursor();
        return;
    } else if (c == '\b') {
        this->clear_cursor();

        if (m_x > 0) {
            m_x -= m_font->width();
        } else if (m_y > 0) {
            m_y -= m_font->height();
            m_x = m_render_context.framebuffer().width() - m_font->width();
        }

        this->render_cursor();
        return;
    } else if (c == '\r') {
        m_x = 0;
        return;
    }

    u8* glyph = m_font->glyph(c);

    u32* framebuffer = m_render_context.framebuffer().buffer();
    for (size_t y = 0; y < m_font->height(); y++) {
        u8 byte = glyph[y];

        for (size_t x = 0; x < m_font->width(); x++) {
            u32 color = (byte & (1 << (7 - x))) ? 0x00AAAAAA : 0x00000000;
            size_t index = (m_y + y) * m_pitch / 4 + (m_x + x);
            
            framebuffer[index] = color;
        }
    }

    m_x += m_font->width();
    this->render_cursor();
}

void Terminal::render(StringView text) {
    for (char c : text) {
        this->render(c);
    }
}

void Terminal::render_cursor(u32 color) {
    u32* framebuffer = m_render_context.framebuffer().buffer();
    for (size_t y = 0; y < m_font->height(); y++) {
        for (size_t x = 0; x < m_font->width() / 2; x++) {
            size_t index = (m_y + y) * m_pitch / 4 + (m_x + x);
            framebuffer[index] = color;
        }
    }
}

void Terminal::scroll() {
    if (m_flanterm_context) {
        return;
    }

    // TODO: Implement proper scrolling
    u32* framebuffer = m_render_context.framebuffer().buffer();
    memset(framebuffer, 0, m_height * m_pitch);

    m_y = 0;
}

}