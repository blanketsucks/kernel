#include <loader/object.h>
#include <loader/loader.h>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

ErrorOr<void> DynamicObject::load() {
    m_fd = open_length(m_name.data(), m_name.size(), O_RDONLY, 0);
    if (m_fd < 0) {
        return Error(errno);
    }

    struct stat st;
    if (fstat(m_fd, &st) < 0) {
        close(m_fd);
        return Error(errno);
    }

    void* data = mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, m_fd, 0);
    if (data == MAP_FAILED) {
        close(m_fd);
        return Error(errno);
    }

    m_image = elf::Image(reinterpret_cast<u8*>(data), st.st_size);
    if (!m_image.parse()) {
        munmap(data, st.st_size);
        close(m_fd);

        return Error("Failed to parse ELF image");
    }

    size_t size = 0;
    uintptr_t base = 0;

    for (size_t i = 0; i < m_image.program_headers(); i++) {
        auto& ph = m_image.program_header(i);
        if (ph.p_type == PT_LOAD) {
            uintptr_t vaddr = std::align_down(ph.p_vaddr, PAGE_SIZE);
            uintptr_t memsz = std::align_up(ph.p_memsz, PAGE_SIZE);

            if (base == 0 || vaddr < base) {
                base = vaddr;
            }

            size += memsz;
            continue;
        }
        
        if (ph.p_type != PT_DYNAMIC) {
            continue;
        }

        m_dynamic_entries.resize(ph.p_filesz / sizeof(Elf64_Dyn));
        std::memcpy(m_dynamic_entries.data(), m_image.data() + ph.p_offset, ph.p_filesz);
    }

    m_memory_offset = DynamicLoader::adjust_memory_offset(base, size);

    TRY(this->map_sections());
    TRY(this->read_dynamic_table());

    for (auto& library : m_required_libraries) {
        Optional<String> path = this->search_library_path(library);
        if (!path.has_value()) {
            return Error("Failed to find library");
        }

        auto object = TRY(DynamicLoader::open_library(path.value()));
    }

    return {};
}

DynamicObject::~DynamicObject() {
    if (m_fd >= 0) {
        close(m_fd);
    }

    if (m_image.data()) {
        munmap(const_cast<u8*>(m_image.data()), m_image.size());
    }
}

uintptr_t DynamicObject::entry() const {
    auto& header = m_image.header();
    return base() + header.e_entry;
}

ErrorOr<void> DynamicObject::map_sections() {
    for (size_t i = 0; i < m_image.program_headers(); i++) {
        auto& ph = m_image.program_header(i);
        if (ph.p_type != PT_LOAD) {
            continue;
        }

        u64 vaddr = std::align_down(ph.p_vaddr, PAGE_SIZE);
        size_t size = std::align_up(ph.p_filesz, PAGE_SIZE);

        int prot = PROT_NONE;
        if (ph.p_flags & PF_X) {
            prot |= PROT_EXEC;
        }
        
        if (ph.p_flags & PF_W) {
            prot |= PROT_WRITE;
        }
        
        if (ph.p_flags & PF_R) {
            prot |= PROT_READ;
        }
        
        off_t offset = std::align_down(ph.p_offset, PAGE_SIZE);

        void* addr = mmap(reinterpret_cast<void*>(base() + vaddr), size, prot, MAP_PRIVATE | MAP_FIXED, m_fd, offset);
        if (addr == MAP_FAILED) {
            return Error(errno);
        }

        if (ph.p_filesz != ph.p_memsz) {
            size_t memsz = std::align_up(ph.p_memsz, PAGE_SIZE);
            void* result = mmap(
                reinterpret_cast<void*>(base() + vaddr + size),
                memsz - size,
                prot,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                -1,
                0
            );

            if (result == MAP_FAILED) {
                return Error(errno);
            }

            std::memset(reinterpret_cast<void*>(base() + vaddr + ph.p_filesz), 0, ph.p_memsz - ph.p_filesz);
        }
    }

    return {};
}

ErrorOr<void> DynamicObject::read_dynamic_table() {
    for (auto& dyn : m_dynamic_entries) {
        switch (dyn.d_tag) {
            case DT_NULL:
                break;
            case DT_STRTAB: {
                m_dynamic_string_table = reinterpret_cast<char*>(base() + dyn.d_un.d_ptr);
                break;
            }
            case DT_STRSZ: {
                m_dynamic_string_table_size = dyn.d_un.d_val;
                break;
            }
            case DT_SYMTAB: {
                m_dynamic_symbol_table = reinterpret_cast<Elf64_Sym*>(base() + dyn.d_un.d_ptr);
                break;
            }
            case DT_SYMENT: {
                if (dyn.d_un.d_val != sizeof(Elf64_Sym)) {
                    dbgln("Unexpected symbol size: {}", dyn.d_un.d_val);
                }
                break;
            }
            case DT_HASH: {
                u32* hash_table = reinterpret_cast<u32*>(base() + dyn.d_un.d_ptr);
                m_dynamic_symbol_count = hash_table[1];

                break;
            }
            case DT_INIT: {
                m_init_function = reinterpret_cast<InitFunction>(base() + dyn.d_un.d_ptr);
                break;
            }
            case DT_INIT_ARRAY: {
                m_init_array = reinterpret_cast<InitFunction*>(base() + dyn.d_un.d_ptr);
                break;
            }
            case DT_INIT_ARRAYSZ: {
                m_init_array_size = dyn.d_un.d_val / sizeof(InitFunction);
                break;
            }
            case DT_FINI: {
                m_fini_function = reinterpret_cast<FiniFunction>(base() + dyn.d_un.d_ptr);
                break;
            }
            case DT_FINI_ARRAY: {
                m_fini_array = reinterpret_cast<FiniFunction*>(base() + dyn.d_un.d_ptr);
                break;
            }
            case DT_FINI_ARRAYSZ: {
                m_fini_array_size = dyn.d_un.d_val / sizeof(FiniFunction);
                break;
            }
            case DT_PLTGOT: {
                m_plt_got = reinterpret_cast<void*>(base() + dyn.d_un.d_ptr);
                break;
            }
            case DT_PLTRELSZ: {
                m_plt_got_size = dyn.d_un.d_val;
                break;
            }
            case DT_PLTREL: {
                m_plt_rel_type = dyn.d_un.d_val;
                break;
            }
            case DT_JMPREL: {
                m_plt_relocation_table = reinterpret_cast<void*>(base() + dyn.d_un.d_ptr);
                break;
            }
            case DT_RELA: {
                m_has_addend = true;
                [[fallthrough]];
            }
            case DT_REL: {
                m_relocation_table = reinterpret_cast<u8*>(base() + dyn.d_un.d_ptr);
                break;
            }
            case DT_RELASZ:
            case DT_RELSZ: {
                m_relocation_count = dyn.d_un.d_val;
                break;
            }
            case DT_RELAENT:
            case DT_RELENT: {
                m_relocation_size = dyn.d_un.d_val;
                break;
            }
            default:
                break;
        }
    }

    for (auto& dyn : m_dynamic_entries) {
        if (dyn.d_tag != DT_NEEDED) {
            continue;
        }

        char* library = m_dynamic_string_table + dyn.d_un.d_val;
        m_required_libraries.append(StringView(library));
    }

    return {};
}

Optional<String> DynamicObject::search_library_path(StringView library) {
    static const Vector<StringView> default_search_paths = {
        "/lib/",
        "/usr/lib/"
    };

    // TODO: Add support for RPATH and RUNPATH
    for (auto& search_path : default_search_paths) {
        String path = format("{}{}", search_path, library);
        struct stat st;

        if (stat_length(path.data(), path.size(), &st) == 0) {
            return path;
        }
    }

    return {};
}

size_t DynamicObject::relocation_count() {
    if (m_relocation_size == 0) {
        return 0;
    }

    return m_relocation_count / m_relocation_size;
}

Elf64_Rela* DynamicObject::relocation(size_t index) {
    size_t offset = index * m_relocation_size;
    return reinterpret_cast<Elf64_Rela*>(m_relocation_table + offset);
}

ErrorOr<void> DynamicObject::perform_relocations() {
    for (size_t i = 0; i < this->relocation_count(); i++) {
        Elf64_Rela* rel = this->relocation(i);
        TRY(this->perform_relocation(rel));
    }

    return {};
}

ErrorOr<void> DynamicObject::perform_plt_relocations() {
    size_t count = m_plt_got_size / sizeof(Elf64_Rela);
    Elf64_Rela* relocs = reinterpret_cast<Elf64_Rela*>(m_plt_relocation_table);

    for (size_t i = 0; i < count; i++) {
        TRY(this->perform_plt_relocation(&relocs[i]));
    }

    u64* got = reinterpret_cast<u64*>(m_plt_got);
    got[2] = 0xABABABA;

    return {};
}

ErrorOr<void> DynamicObject::perform_relocation(Elf64_Rela* relocation) {
    uintptr_t* patch = reinterpret_cast<uintptr_t*>(base() + relocation->r_offset);
    u32 type = ELF64_R_TYPE(relocation->r_info);

    switch (type) {
        case R_X86_64_RELATIVE: {
            if (m_has_addend) {
                *patch = base() + relocation->r_addend;
            } else {
                *patch = base() + *patch;
            }

            break;
        }
        case R_X86_64_GLOB_DAT: {
            Elf64_Sym* symbol = &m_dynamic_symbol_table[ELF64_R_SYM(relocation->r_info)];
            u8 bind = ELF64_ST_BIND(symbol->st_info);

            if (bind == STB_WEAK) {
                StringView name = StringView(m_dynamic_string_table + symbol->st_name);
                uintptr_t address = DynamicLoader::get_symbol_address(name);

                *patch = address;
                break;
            }

            *patch = base() + symbol->st_value;
            break;
        }
        case R_X86_64_COPY: {
            Elf64_Sym* symbol = &m_dynamic_symbol_table[ELF64_R_SYM(relocation->r_info)];
            StringView name = StringView(m_dynamic_string_table + symbol->st_name);

            uintptr_t address = DynamicLoader::get_symbol_address(name);
            if (!address) {
                std::memcpy(patch, reinterpret_cast<void*>(base() + symbol->st_value), symbol->st_size);
            } else {
                std::memcpy(patch, reinterpret_cast<void*>(address), symbol->st_size);
            }

            break;
        }
        default: {
            dbgln("Unsupported relocation type: {}", type);
        }
    }

    return {};
}

ErrorOr<void> DynamicObject::perform_plt_relocation(Elf64_Rela* relocation) {
    uintptr_t* patch = reinterpret_cast<uintptr_t*>(base() + relocation->r_offset);
    Elf64_Sym* symbol = &m_dynamic_symbol_table[ELF64_R_SYM(relocation->r_info)];

    u8 bind = ELF64_ST_BIND(symbol->st_info);

    StringView name = StringView(m_dynamic_string_table + symbol->st_name);
    if (!symbol->st_value || bind == STB_WEAK) {
        uintptr_t address = DynamicLoader::get_symbol_address(name);
        *patch = address ? address : base() + symbol->st_value;
    } else {
        *patch = base() + symbol->st_value;
    }

    if (!*patch) {
        dbgln("Failed to resolve PLT symbol: {}", name);
    }

    return {};
}

void DynamicObject::invoke_init() {
    if (m_init_function) {
        m_init_function();
    }

    if (!m_init_array) {
        return;
    }

    for (size_t i = 0; i < m_init_array_size; i++) {
        if (!m_init_array[i]) {
            continue;
        }

        m_init_array[i]();
    }
}

void DynamicObject::invoke_fini() {
    if (m_fini_function) {
        m_fini_function();
    }

    if (!m_fini_array) {
        return;
    }

    for (size_t i = 0; i < m_fini_array_size; i++) {
        if (!m_fini_array[i]) {
            continue;
        }

        m_fini_array[i]();
    }
}

Elf64_Sym* DynamicObject::find_symbol(StringView name) {
    for (size_t i = 0; i < m_dynamic_symbol_count; i++) {
        Elf64_Sym* symbol = &m_dynamic_symbol_table[i];
        StringView symbol_name = StringView(m_dynamic_string_table + symbol->st_name);

        if (symbol_name == name && symbol->st_value != 0) {
            return symbol;
        }
    }

    return nullptr;
}