#pragma once

#include <std/string.h>
#include <std/string_view.h>

namespace kernel {

struct Symbol {
    char* name;
    u32 address;
};

bool has_loaded_symbols();

void parse_symbols(StringView buffer);
void parse_symbols_from_fs();

Symbol* resolve_symbol(const char* name);
Symbol* resolve_symbol(u32 address);

}