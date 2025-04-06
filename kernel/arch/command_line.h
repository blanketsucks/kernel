#pragma once

#include <kernel/common.h>
#include <kernel/arch/boot_info.h>

#include <std/optional.h>
#include <std/hash_map.h>

namespace kernel {

class CommandLine {
public:
    static void init();
    static CommandLine* instance() { return s_instance; }

    [[nodiscard]] StringView root() const;

private:
    static CommandLine* s_instance;

    CommandLine();
    void parse(StringView cmdline);

    bool has(StringView key) const;
    Optional<StringView> get(StringView key) const;

    HashMap<StringView, StringView> m_args;
};

}