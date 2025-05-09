#pragma once

#include <std/vector.h>
#include <std/string.h>
#include <std/string_view.h>
#include <std/function.h>
#include <std/format.h>

#include <libgfx/render_context.h>
#include <libgfx/font.h>

#include <flanterm/backends/fb.h>

namespace shell {

class Command {
public:
    Command(String name, Vector<String> args);

    StringView name() const { return m_name; }
    Vector<String> const& args() const { return m_args; }

    char* pathname() const { return m_pathname; }

    char** argv() const;
    size_t argc() const { return m_args.size() + 1; }

private:
    String m_name;
    Vector<String> m_args;

    char* m_pathname;
};

Command parse_shell_command(String line);

struct Line {
    String text;
    size_t prompt;
};

class Terminal {
public:
    Terminal(u32 width, u32 height, gfx::RenderContext&, u8*, size_t, size_t);

    static String cwd();

    Line& current_line();
    Line const& current_line() const;

    void on_char(char);
    void clear();
    
    void add_line(String text, bool newline_before = false);

    void write(StringView text);
    void writeln(StringView text);

    void advance_line() {
        m_current_line++;
    }

    Function<void(String)> on_line_flush;

private:
    u32 m_width;
    u32 m_height;

    flanterm_context* m_context;
    gfx::RenderContext& m_render_context;

    Vector<Line> m_lines;

    u32 m_current_line = 0;
};

}