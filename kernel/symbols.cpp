#include <kernel/symbols.h>
#include <kernel/fs/vfs.h>

#include <std/vector.h>
#include <std/format.h>

namespace kernel {

struct MapEntry {
    u64 address;
    u32 length;
    char name[];
} PACKED;

static Vector<Symbol> s_symbols;
static bool s_loaded_symbols = false;

bool has_loaded_symbols() {
    return s_loaded_symbols;
}

Symbol* resolve_symbol(StringView name) {
    if (!s_loaded_symbols) {
        return nullptr;
    }

    for (auto& symbol : s_symbols) {
        if (symbol.name == name) {
            return &symbol;
        }
    }

    return nullptr;
}

Symbol* resolve_symbol(FlatPtr address) {
    if (!s_loaded_symbols) {
        return nullptr;
    }

    for (u32 i = 0; i < s_symbols.size() - 1; i++) {
        if (address < s_symbols[i + 1].address) {
            return &s_symbols[i];
        }
    }

    return nullptr;
}

void parse_symbols_from_inode(fs::Inode& inode) {
    u8* buffer = new u8[inode.size()];
    inode.read(buffer, inode.size(), 0);

    MapEntry* entry = reinterpret_cast<MapEntry*>(buffer);
    size_t offset = 0;

    while (entry->address) {
        char* name = new char[entry->length];
        memcpy(name, entry->name, entry->length);

        s_symbols.append(Symbol { { name, entry->length }, static_cast<FlatPtr>(entry->address) });

        offset += sizeof(MapEntry) + entry->length;
        entry = reinterpret_cast<MapEntry*>(buffer + offset);
    }

    s_symbols.shrink_to_fit();
}

void parse_symbols() {
    auto vfs = fs::vfs();

    auto result = vfs->resolve("/boot/kernel.map");
    if (result.is_err()) {
        return;
    }

    auto resolved = result.value();
    auto& inode = resolved->inode();

    parse_symbols_from_inode(inode);
    s_loaded_symbols = true;
}

}