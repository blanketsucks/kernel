#include <loader/loader.h>

static HashMap<String, RefPtr<DynamicObject>> s_global_libraries;
static RefPtr<DynamicObject> s_executable;

static HashMap<String, uintptr_t> s_global_symbols;

static uintptr_t s_memory_offset = 0;

size_t DynamicLoader::adjust_memory_offset(uintptr_t base, size_t size) {
    if (!s_memory_offset) {
        base = std::align_up(base, PAGE_SIZE);
        s_memory_offset = base + std::align_up(size, PAGE_SIZE);
        
        return 0;
    }

    size_t offset = s_memory_offset;
    s_memory_offset += std::align_up(size, PAGE_SIZE);

    return offset;
}

HashMap<String, RefPtr<DynamicObject>>& DynamicLoader::libraries() {
    return s_global_libraries;
}

RefPtr<DynamicObject> DynamicLoader::executable() {
    return s_executable;
}

ErrorOr<RefPtr<DynamicObject>> DynamicLoader::open_main_executable(const String& name) {
    auto object = DynamicObject::create(name);
    TRY(object->load());

    s_executable = object;
    return object;
}

ErrorOr<RefPtr<DynamicObject>> DynamicLoader::open_library(const String& name) {
    auto iterator = s_global_libraries.find(name);
    if (iterator != s_global_libraries.end()) {
        return iterator->value;
    }

    auto object = DynamicObject::create(name);
    s_global_libraries.set(name, object);

    TRY(object->load());

    TRY(object->perform_relocations());
    TRY(object->perform_plt_relocations());

    return object;
}

uintptr_t DynamicLoader::get_symbol_address(const String& name) {    
    auto iterator = s_global_symbols.find(name);
    if (iterator != s_global_symbols.end()) {
        return iterator->value;
    }
    
    for (auto& [_, object] : s_global_libraries) {
        auto symbol = object->find_symbol(name);
        if (symbol) {
            return object->base() + symbol->st_value;
        }
    }

    return 0;
}

void DynamicLoader::add_symbol(const String& name, uintptr_t address) {
    s_global_symbols.set(name, address);
}