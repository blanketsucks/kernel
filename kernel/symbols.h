#pragma once

#include <std/string.h>
#include <std/string_view.h>

namespace kernel {

struct Symbol {
    StringView name;
    FlatPtr address;
};

bool has_loaded_symbols();

void parse_symbols();

Symbol* resolve_symbol(StringView name);
Symbol* resolve_symbol(FlatPtr address);

}