#include <kernel/symbols.h>
#include <kernel/fs/vfs.h>

#include <std/vector.h>

namespace kernel {

static Vector<Symbol> s_symbols;

Symbol* resolve_symbol(const char* name) {
    for (auto& symbol : s_symbols) {
        if (std::strcmp(symbol.name, name) == 0) {
            return &symbol;
        }
    }

    return nullptr;
}

Symbol* resolve_symbol(u32 address) {
    for (u32 i = 0; i < s_symbols.size() - 1; i++) {
        if (address < s_symbols[i + 1].address) {
            return &s_symbols[i];
        }
    }

    return nullptr;
}

void parse_symbols(StringView symbols) {
    while (!symbols.empty()) {
        size_t newline = symbols.find('\n');
        if (newline == StringView::npos) {
            break;
        }

        StringView line = symbols.substr(0, newline);
        symbols = symbols.substr(newline + 1);

        size_t index = line.find(' ');
        if (index == StringView::npos) {
            continue;
        }

        StringView address = line.substr(0, index);
        StringView name = line.substr(index + 1);

        char* buffer = new char[name.size() + 1];
        memcpy(buffer, name.data(), name.size());

        buffer[name.size()] = '\0';
        u32 addr = std::strntoul(address.data(), address.size(), nullptr, 16);

        s_symbols.append(Symbol { buffer, addr });
    }
}

void parse_symbols_from_fs() {
    auto vfs = fs::vfs();

    auto result = vfs->resolve("/boot/kernel.map");
    if (result.is_err()) {
        return;
    }

    auto resolved = result.value();
    auto& inode = resolved->inode();

    char* buffer = new char[inode.size()];
    inode.read(buffer, inode.size(), 0);

    parse_symbols({ buffer, inode.size() });
}

}