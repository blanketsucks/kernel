#pragma once

#include <std/types.h>
#include <std/elf.h>
#include <std/string_view.h>

namespace elf {

class Image;

class Symbol {

};

class Section {

};

class Image {
public:
    static constexpr u32 ELF_64_MAGIC = 0x464C457F;

    Image(const u8* data, size_t size) : m_data(data), m_size(size) {}
    Image() = default;

    bool parse();

    size_t symbols() const;
    size_t sections() const;
    size_t program_headers() const;

    Elf64_Ehdr const& header() const;

    const Elf64_Shdr& section_header(size_t index) const;
    const Elf64_Phdr& program_header(size_t index) const;

    StringView string_table(u32 index) const;
    StringView section_header_string_table(u32 index) const;

private:
    template<typename T>
    const T* offset(size_t offset) const {
        return reinterpret_cast<T const*>(m_data + offset);
    }

    const u8* m_data = nullptr;
    size_t m_size = 0;

    size_t m_string_table_offset = 0;
    size_t m_symbol_table_offset = 0;
};

}