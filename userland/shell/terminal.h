#pragma once

#include <std/vector.h>
#include <std/string.h>
#include <std/string_view.h>
#include <std/function.h>
#include <std/format.h>

#include <libgfx/render_context.h>
#include <libgfx/fonts/psf.h>
#include <libgfx/font.h>

#include <flanterm/backends/fb.h>

namespace shell {

struct Line {
    String text;
    size_t prompt;
};

class Terminal {
public:
    Terminal(gfx::RenderContext& context, RefPtr<gfx::PSFFont> font, bool use_flanterm = false);

    static String cwd();

    Line& current_line();
    Line const& current_line() const;

    void on_char(char);
    void clear();
    
    void add_line(String text, bool newline_before = false);
    void replace_current_line(const String& text);

    void write(StringView text);
    void writeln(StringView text);

    void advance_line() {
        m_current_line++;
    }

    Function<void(String)> on_line_flush;

    void render(char);
    void render(StringView);

    void render_cursor(u32 color = 0x00AAAAAA);
    void clear_cursor() { this->render_cursor(0); }

    void scroll();

private:
    gfx::RenderContext& m_render_context;
    RefPtr<gfx::PSFFont> m_font;

    flanterm_context* m_flanterm_context = nullptr;

    Vector<Line> m_lines;

    u32 m_current_line = 0;

    u32 m_x = 0;
    u32 m_y = 0;

    u32 m_width;
    u32 m_height;
    u32 m_pitch;

    u32 m_rows;
    u32 m_cols;
};

}