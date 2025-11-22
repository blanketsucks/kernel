#pragma once

#include <kernel/common.h>
#include <kernel/boot/boot_info.h>

#include <std/optional.h>
#include <std/hash_map.h>

namespace kernel {

class CommandLine {
public:
    static void initialize();
    static CommandLine* instance();

    [[nodiscard]] StringView root() const;
    [[nodiscard]] StringView init() const;

private:
    void parse(StringView cmdline);

    bool has(StringView key) const;
    Optional<StringView> get(StringView key) const;

    HashMap<StringView, StringView> m_args;
};

}