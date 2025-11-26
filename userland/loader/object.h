#pragma once

#include <libelf/image.h>

#include <std/string.h>
#include <std/memory.h>
#include <std/result.h>
#include <std/vector.h>
#include <std/optional.h>

class DynamicLoader;

class DynamicObject {
public:
    using InitFunction = void(*)();
    using FiniFunction = void(*)();

    static RefPtr<DynamicObject> create(const String& name) {
        return RefPtr<DynamicObject>(new DynamicObject(name));
    };

    ~DynamicObject();

    uintptr_t base() const { return m_memory_offset; }

    elf::Image& image() { return m_image; }
    uintptr_t entry() const;

    Optional<String> search_library_path(StringView library);

    ErrorOr<void> map_sections();
    ErrorOr<void> read_dynamic_table();

    ErrorOr<void> perform_relocations();
    ErrorOr<void> perform_plt_relocations();

    void invoke_init();
    void invoke_fini();

    Elf64_Sym* find_symbol(StringView name);
    Elf64_Sym* find_symbol(size_t index);

private:
    friend class DynamicLoader;

    DynamicObject(const String& name) : m_name(name) {}

    ErrorOr<void> load();

    size_t relocation_count();
    Elf64_Rela* relocation(size_t index);

    ErrorOr<void> perform_relocation(Elf64_Rela* relocation);
    ErrorOr<void> perform_plt_relocation(Elf64_Rela* relocation);

    int m_fd = -1;
    elf::Image m_image;

    String m_name;

    Vector<Elf64_Dyn> m_dynamic_entries;
    size_t m_memory_offset = 0;

    char* m_dynamic_string_table = nullptr;
    size_t m_dynamic_string_table_size = 0;

    Elf64_Sym* m_dynamic_symbol_table = nullptr;
    size_t m_dynamic_symbol_count = 0;

    Vector<StringView> m_required_libraries;

    InitFunction m_init_function = nullptr;
    InitFunction* m_init_array = nullptr;
    size_t m_init_array_size = 0;

    FiniFunction m_fini_function = nullptr;
    FiniFunction* m_fini_array = nullptr;
    size_t m_fini_array_size = 0;

    u8* m_relocation_table = nullptr;
    size_t m_relocation_count = 0;
    size_t m_relocation_size = 0;

    void* m_plt_got = nullptr;
    size_t m_plt_got_size = 0;
    u32 m_plt_rel_type = 0;

    void* m_plt_relocation_table = nullptr;

    bool m_has_addend = false;
};