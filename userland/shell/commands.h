#pragma once

#include <std/vector.h>
#include <std/string.h>

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

};