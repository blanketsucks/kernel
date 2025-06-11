#pragma once

#include <kernel/common.h>

#include <kernel/fs/fd.h>

#include <std/memory.h>
#include <std/vector.h>
#include <std/string.h>
#include <std/elf.h>

namespace kernel {

constexpr u32 ELF_MAGIC = 0x464C457F;

#ifdef __x86__
    using ELFHeader = Elf32_Ehdr;
    using ELFPHeader = Elf32_Phdr;
#else
    using ELFHeader = Elf64_Ehdr;
    using ELFPHeader = Elf64_Phdr;
#endif

class ELF {
public:
    ELF(RefPtr<fs::FileDescriptor> file) : m_file(file) {}

    static ErrorOr<RefPtr<ELF>> create(RefPtr<fs::FileDescriptor> file);

    fs::FileDescriptor& file() { return *m_file; }
    ELFHeader const* header() const { return m_header; }

    FlatPtr entry() const {
        return m_header ? m_header->e_entry : 0;
    }

    String const& interpreter() const { return m_interpreter; }
    bool has_interpreter() const { return !m_interpreter.empty(); }

    Vector<ELFPHeader> const& program_headers() const { return m_program_headers; }

    ErrorOr<void> load();
    ErrorOr<void> read_header();

    void read_program_headers();
private:
    ELFHeader* m_header = nullptr;
    RefPtr<fs::FileDescriptor> m_file;

    String m_interpreter;
    Vector<ELFPHeader> m_program_headers;    
};

}