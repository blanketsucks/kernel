#pragma once

#include <kernel/common.h>

#include <kernel/fs/file.h>

#include <std/memory.h>
#include <std/vector.h>
#include <std/string.h>
#include <std/elf.h>

namespace kernel {

constexpr u32 ELF_MAGIC = 0x464C457F;

class ELF {
public:
    ELF(OwnPtr<fs::File> file) : m_file(move(file)) {}

    fs::File& file() { return *m_file; }
    Elf32_Ehdr const* header() const { return m_header; }

    String const& interpreter() const { return m_interpreter; }
    bool has_interpreter() const { return !m_interpreter.empty(); }

    Vector<Elf32_Phdr> const& program_headers() const { return m_program_headers; }

    bool read_header();
    void read_program_headers();
private:
    Elf32_Ehdr* m_header = nullptr;
    OwnPtr<fs::File> m_file;

    String m_interpreter;
    Vector<Elf32_Phdr> m_program_headers;    
};

}