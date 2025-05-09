#include <shell/terminal.h>
#include <std/format.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

namespace shell {

void _free(void* ptr, size_t) {
    free(ptr);
}

Command parse_shell_command(String line) {
    Vector<String> args;
    String name;

    size_t index = line.find(' ');

    if (index == StringView::npos) {
        return { line, move(args) };
    } else {
        name = line.substr(0, index);
        line = line.substr(index + 1);
    }

    while (!line.empty()) {
        bool in_quotes = false;
        if (line.startswith('"')) {
            in_quotes = true;
            line = line.substr(1);
        }

        if (in_quotes) {
            index = line.find('"');
        } else {
            index = line.find(' ');
        }

        if (index == StringView::npos) {
            args.append(line);
            break;
        }

        args.append(line.substr(0, index));
        line = line.substr(index + 1);
    }

    return { move(name), move(args) };
}

Command::Command(String name, Vector<String> args) : m_name(move(name)), m_args(move(args)) {
    m_pathname = new char[m_name.size() + 1];
    memcpy(m_pathname, m_name.data(), m_name.size());
    m_pathname[m_name.size()] = '\0';
}

char** Command::argv() const {
    char** argv = new char*[m_args.size() + 2];
    for (size_t i = 0; i < m_args.size(); i++) {
        auto& argument = m_args[i];

        char* buffer = new char[m_args[i].size() + 1];
        memcpy(buffer, argument.data(), argument.size());

        buffer[argument.size()] = '\0';
        argv[i + 1] = buffer;
    }

    argv[0] = m_pathname;
    argv[m_args.size() + 1] = nullptr;

    return argv;
}

Terminal::Terminal(
    u32 width, u32 height, gfx::RenderContext& context, u8* font, size_t font_width, size_t font_height
) : on_line_flush(nullptr), m_width(width), m_height(height), m_render_context(context) {
    m_context = flanterm_fb_init(
        malloc, _free,
        context.framebuffer().buffer(), width, height, context.framebuffer().width() * sizeof(u32),
        8, 16,
        8, 8,
        8, 0,
        nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        font, font_width, font_height, 1,
        0, 0,
        0
    );

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
        flanterm_write(m_context, "\n", 1);
    }

    flanterm_write(m_context, line.data(), line.size());
    m_lines.append({ move(line), prompt_length });
}

void Terminal::write(StringView text) {
    flanterm_write(m_context, text.data(), text.size());
}

void Terminal::writeln(StringView text) {
    write(text);
    flanterm_write(m_context, "\n", 1);
}

void Terminal::on_char(char c) {
    auto& line = current_line();
    if (c == '\n') {
        flanterm_write(m_context, "\n", 1);

        this->on_line_flush(line.text.substr(line.prompt));
        this->add_line({});

        m_current_line++;
        return;
    } else if (c == '\b') {
        if (line.text.size() <= line.prompt) {
            return;
        }

        line.text.pop();
        flanterm_write(m_context, "\b \b", 3);
    } else {
        line.text.append(c);
        flanterm_write(m_context, &c, 1);
    }
}

void Terminal::clear() {
    m_lines.clear();
    this->add_line({});
    
    m_current_line = 0;

    gfx::Rect rect = { 0, 0, (int)m_width, (int)m_height };
    rect.draw(m_render_context, 0x000000);
}

}