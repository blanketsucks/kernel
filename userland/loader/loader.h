#pragma once

#include <std/hash_map.h>

#include <loader/object.h>

static constexpr size_t PAGE_SIZE = 0x1000;

class DynamicLoader {
public:
    static size_t adjust_memory_offset(uintptr_t base, size_t size);

    static HashMap<String, RefPtr<DynamicObject>>& libraries();
    static RefPtr<DynamicObject> executable();

    static ErrorOr<RefPtr<DynamicObject>> open_main_executable(const String& name);
    static ErrorOr<RefPtr<DynamicObject>> open_library(const String& library);

    static void add_symbol(const String& name, uintptr_t address);

    static uintptr_t get_symbol_address(const String& name);
};