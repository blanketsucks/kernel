#pragma once

#include <std/vector.h>
#include <std/string.h>
#include <std/function.h>

#include <libgfx/render_context.h>
#include <libgfx/font.h>

namespace shell {

struct Command {
    String name;
    Vector<String> args;

    char** argv() const;
    size_t argc() const;
};

Command parse_shell_command(String line);

struct Line {
    String text;

    bool dirty;
    bool did_backspace;

    size_t prompt_length;
};

class Terminal {
public:
    Terminal(u32 width, u32 height, gfx::RenderContext&, RefPtr<gfx::Font>);

    static String cwd();

    Line& current_line();
    Line const& current_line() const;

    void on_char(char);
    void clear();
    
    void add_line(String text);
    void add_line_without_prompt(String text);

    void advance_line() {
        m_current_line++;
    }

    void render(int size = 16);

    Function<void(String)> on_line_flush;

private:
    u32 m_width;
    u32 m_height;

    gfx::RenderContext& m_render_context;
    RefPtr<gfx::Font> m_font;

    Vector<Line> m_lines;
    u32 m_current_line = 0;
};

}