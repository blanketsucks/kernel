#pragma once

#include <std/vector.h>
#include <std/string.h>
#include <std/string_view.h>
#include <std/function.h>
#include <std/format.h>

#include <libgfx/render_context.h>
#include <libgfx/fonts/psf.h>
#include <libgfx/font.h>

namespace shell {

struct Line {
    String text;
    size_t prompt;
};

struct Cell {
    u32 bg, fg;
    char c;
};

class Terminal {
public:
    static constexpr u32 DEFAULT_FG = 0x00AAAAAA;
    static constexpr u32 DEFAULT_BG = 0x00000000;

    Terminal(gfx::RenderContext& context, RefPtr<gfx::PSFFont> font);
    
    Function<void(String)> on_line_flush;

    static String cwd();

    Line& current_line();
    Line const& current_line() const;

    void on_char(char);
    void clear();
    
    void add_line(String text, bool newline_before = false);
    void replace_current_line(const String& text);

    void write(StringView text);
    void writeln(StringView text);

    void render_cell(size_t row, size_t col, Cell const& cell);
    void render_cursor(u32 color = DEFAULT_FG);
    void render();

    void scroll();

    void push(StringView text);
    void push(char c, u32 fg = DEFAULT_FG, u32 bg = DEFAULT_BG);

private:
    gfx::RenderContext& m_render_context;
    RefPtr<gfx::PSFFont> m_font;

    Vector<Line> m_lines;
    Vector<Cell> m_cells;

    u32 m_current_line = 0;

    u32 m_width;
    u32 m_height;
    u32 m_pitch;

    size_t m_current_row = 0;
    size_t m_current_col = 0;

    u32 m_rows;
    u32 m_cols;
};

}