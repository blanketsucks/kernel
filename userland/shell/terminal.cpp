#include <shell/terminal.h>
#include <std/format.h>
#include <string.h>
#include <unistd.h>

namespace shell {

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

char** Command::argv() const {
    char** argv = new char*[args.size() + 2];
    for (size_t i = 0; i < args.size(); i++) {
        auto& argument = args[i];

        char* buffer = new char[args[i].size() + 1];
        memcpy(buffer, argument.data(), argument.size());

        buffer[argument.size()] = '\0';
        argv[i + 1] = buffer;
    }

    argv[0] = const_cast<char*>(name.data());
    argv[args.size() + 1] = nullptr;

    return argv;
}

size_t Command::argc() const { return args.size() + 1; }

Terminal::Terminal(gfx::RenderContext& context, RefPtr<gfx::Font> font) : on_line_flush(nullptr), m_render_context(context), m_font(font) {
    this->add_line({});
}

String Terminal::cwd() {
    char buffer[256];
    getcwd(buffer, sizeof(buffer));

    return String(buffer);
}

Line& Terminal::current_line() { return m_lines[m_current_line]; }
Line const& Terminal::current_line() const { return m_lines[m_current_line]; }

void Terminal::add_line(String text) {
    String line = format("{} $ {}", cwd(), text);
    size_t prompt_length = line.size() - text.size();

    m_lines.append({ move(line), true, false, prompt_length });
}

void Terminal::add_line_without_prompt(String text) {
    m_lines.append({ move(text), true, false, 0 });
}

void Terminal::render(int size) {
    auto box = m_font->measure(current_line().text, size);

    int y = box.y;
    int width = m_render_context.framebuffer().width();

    for (auto& line : m_lines) {
        if (line.text.empty()) {
            y += box.height;
            continue;
        }

        auto bbox = m_font->measure(line.text, size);
        if (!bbox.height) {
            bbox.height = box.height;
        }

        if (line.dirty) {
            // Clear out the whole line only if we backspaced
            if (line.did_backspace) {
                gfx::Rect rect = { 0, y - box.y, width, bbox.height };
                rect.draw(m_render_context, 0x000000);

                line.did_backspace = false;
            }

            m_font->render(m_render_context, { 0, y }, 0xFFFFFFFF, line.text, size);
            line.dirty = false;
        }

        y += bbox.height;
        box = bbox;
    }
}

void Terminal::on_char(char c) {
    auto& line = current_line();
    if (c == '\n') {
        this->on_line_flush(line.text.substr(line.prompt_length));

        this->add_line({});
        m_current_line++;
        
        line.dirty = false;
        return;
    } else if (c == '\b') {
        if (line.text.size() <= line.prompt_length) {
            return;
        }

        line.did_backspace = true;
        line.text.pop();
    } else {
        line.text.append(c);
    }

    line.dirty = true;
}

}