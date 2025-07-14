#include <shell/commands.h>

#include <string.h>

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

}