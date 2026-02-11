#include <shell/terminal.h>
#include <std/format.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

namespace shell {

Terminal::Terminal(
    gfx::RenderContext& context, RefPtr<gfx::PSFFont> font
) : on_line_flush(nullptr), m_render_context(context), m_font(move(font)) {
    auto& fb = m_render_context.framebuffer();
    
    m_height = fb.height();
    m_width = fb.width();
    m_pitch = fb.pitch();

    m_rows = m_height / m_font->height();
    m_cols = m_width / m_font->width();

    m_cells.resize(m_rows * m_cols);

    this->fetch_cwd();
    this->add_line({});
}

void Terminal::fetch_cwd() {
    char buffer[4096];
    if (getcwd(buffer, 4096)) {
        m_cwd = String(buffer);
    } else {
        m_cwd = "/";
    }
}

Line& Terminal::current_line() { return m_lines[m_current_line]; }
Line const& Terminal::current_line() const { return m_lines[m_current_line]; }

void Terminal::add_line(StringView text, bool newline_before) {
    String line = format("{} $ {}", m_cwd, text);
    size_t prompt_length = line.size() - text.size();

    if (newline_before) {
        this->push('\n');
    }

    this->push(line);
    m_lines.append({ move(line), prompt_length });

    this->render();
}

void Terminal::write(StringView text) {
    this->push(text);
    this->render();
}

void Terminal::writeln(StringView text) {
    this->push(text);
    this->push('\n');

    this->render();
}

void Terminal::on_char(char c) {
    auto& line = current_line();
    if (c == '\n') {
        this->push('\n');

        this->on_line_flush(line.text.substr(line.prompt));
        this->add_line({});

        m_current_line++;
    } else if (c == '\b') {
        if (line.text.size() == line.prompt) {
            return;
        }

        line.text.pop();
        this->push("\b \b");
    } else {
        line.text.append(c);
        this->push(c);
    }

    this->render();
}

void Terminal::clear() {
    m_lines.clear();
    this->add_line({});
    
    m_current_line = 0;
}

void Terminal::replace_current_line(const String& text) {
    auto& line = this->current_line();
    for (size_t i = 0; i < line.text.size(); i++) {
        this->push("\b \b");
    }

    line.text = format("{} $ {}", m_cwd, text);
    line.prompt = line.text.size() - text.size();

    this->push(line.text);
    this->render();
}

void Terminal::render_cell(size_t row, size_t col, const Cell& cell) {
    u8* glyph = m_font->glyph(cell.c);
    if (!glyph) {
        glyph = m_font->glyph(0);
    }

    u32* framebuffer = m_render_context.framebuffer().buffer();

    size_t y = row * m_font->height();
    size_t x = col * m_font->width();

    for (size_t dy = 0; dy < m_font->height(); dy++) {
        u8 byte = glyph[dy];

        for (size_t dx = 0; dx < m_font->width(); dx++) {
            u32 color = (byte & (1 << (7 - dx))) ? cell.fg : cell.bg;
            size_t index = (y + dy) * m_pitch / 4 + (x + dx);
            
            framebuffer[index] = color;
        }
    }
}

void Terminal::render() {
    for (size_t row = 0; row < m_rows; row++) {
        for (size_t col = 0; col < m_cols; col++) {
            size_t index = col + row * m_cols;
            auto& cell = m_cells[index];

            if (cell.dirty) {
                this->render_cell(row, col, m_cells[index]);
                cell.dirty = false;
            }
        }
    }

    this->render_cursor();
}

void Terminal::render_cursor(u32 color) {
    size_t height = m_font->height();
    size_t width = m_font->width() / 2;

    size_t x = m_current_col * m_font->width();
    size_t y = m_current_row * m_font->height();

    auto& cell = m_cells[m_current_col + m_current_row * m_cols];
    cell.dirty = true;

    this->fill(x, y, height, width, color);
}

void Terminal::fill(size_t x, size_t y, size_t height, size_t width, u32 color) {
    u32* framebuffer = m_render_context.framebuffer().buffer();

    for (size_t dy = 0; dy < height; dy++) {
        for (size_t dx = 0; dx < width; dx++) {
            size_t index = (y + dy) * m_pitch / 4 + (x + dx);
            framebuffer[index] = color;
        }
    }
}

void Terminal::scroll() {
    // We don't have to re-render everything here, since we know what ever calls push() will call render() later.
    memmove(m_cells.data(), m_cells.data() + m_cols, (m_rows - 1) * m_cols * sizeof(Cell));
    memset(m_cells.data() + (m_rows - 1) * m_cols, 0, m_cols * sizeof(Cell));

    for (auto& cell : m_cells) {
        cell.dirty = true;
    }
}

void Terminal::push(char c, u32 fg, u32 bg) {
    switch (c) {
        case '\n': {
            m_current_col = 0;
            m_current_row++;
            if (m_current_row >= m_rows) {
                m_current_row = m_rows - 1;
                this->scroll();
            }

            return;
        }
        case '\r': {
            m_current_col = 0;
            return;
        }
        case '\b': {
            if (m_current_col > 0) {
                m_current_col--;
            }
            return;
        }
    }

    auto& cell = m_cells[m_current_col + m_current_row * m_cols];

    cell.bg = bg;
    cell.fg = fg;
    cell.c = c;
    cell.dirty = true;

    m_current_col++;
    if (m_current_col < m_cols) {
        return;
    }

    m_current_col = 0;
    m_current_row++;

    if (m_current_row >= m_rows) {
        m_current_row = m_rows - 1;
        this->scroll();
    }
}

void Terminal::push(StringView text) {
    for (char c : text) {
        this->push(c);
    }
}

}