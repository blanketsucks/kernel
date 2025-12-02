#pragma once

#include <std/vector.h>
#include <std/string.h>

namespace shell {

class Command {
public:
    Command(String name, Vector<String> args);
    ~Command();

    StringView name() const { return m_name; }
    Vector<String> const& args() const { return m_args; }

    char* pathname() const { return m_pathname; }

    char** argv() const { return m_argv; }
    size_t argc() const { return m_args.size() + 1; }

private:
    String m_name;
    Vector<String> m_args;

    char** create_argv() const;

    char** m_argv;
    char* m_pathname;
};

Command parse_command_arguments(StringView name, StringView line);
StringView parse_command_name(StringView line);

};