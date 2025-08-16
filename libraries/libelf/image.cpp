#include <libelf/image.h>

namespace elf {

bool Image::parse() {
    u32 magic = *offset<u32>(0);
    if (magic != ELF_64_MAGIC) {
        return false;
    }

    for (size_t i = 0; i < this->sections(); i++) {
        auto& section = this->section_header(i);
        if (section.sh_type == SHT_STRTAB) {
            m_string_table_offset = section.sh_offset;
            break;
        } else if (section.sh_type == SHT_SYMTAB) {
            m_symbol_table_offset = section.sh_offset;
            break;
        }
    }

    return true;
}

size_t Image::symbols() const {
    return m_symbol_table_offset;
}

size_t Image::sections() const {
    return header().e_shnum;
}

size_t Image::program_headers() const {
    return header().e_phnum;
}

Elf64_Ehdr const& Image::header() const {
    return *offset<Elf64_Ehdr>(0);
}

const Elf64_Shdr& Image::section_header(size_t index) const {
    auto& header = this->header();
    return *offset<Elf64_Shdr>(header.e_shoff + index * header.e_shentsize);
}

const Elf64_Phdr& Image::program_header(size_t index) const {
    auto& header = this->header();
    return *offset<Elf64_Phdr>(header.e_phoff + index * header.e_phentsize);
}

StringView Image::string_table(u32 index) const {
    return StringView(offset<char>(m_string_table_offset + index));
}

StringView Image::section_header_string_table(u32 index) const {
    auto& header = this->header();
    auto& section = this->section_header(header.e_shstrndx);

    return StringView(offset<char>(section.sh_offset + index));
}

}