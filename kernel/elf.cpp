#include <kernel/elf.h>
#include <kernel/serial.h>
#include <kernel/posix/unistd.h>

namespace kernel {

bool ELF::read_header() {
    if (m_header) {
        return true;
    }

    auto* header = m_header = new Elf32_Ehdr;

    m_file->seek(0, SEEK_SET);
    m_file->read(header, sizeof(Elf32_Ehdr));

    u32 magic = *reinterpret_cast<u32*>(header);
    if (magic != ELF_MAGIC) {
        return false;
    } else if (header->e_ident[EI_CLASS] != ELFCLASS32) {
        return false;
    }

    return true;
}

void ELF::read_program_headers() {
    if (!m_header || !m_program_headers.empty()) {
        return;
    }

    m_program_headers.resize(m_header->e_phnum);

    m_file->seek(m_header->e_phoff, SEEK_SET);
    m_file->read(m_program_headers.data(), m_header->e_phentsize * m_header->e_phnum);

    for (auto& ph : m_program_headers) {
        if (ph.p_type != PT_INTERP) {
            continue;
        }

        m_interpreter.resize(ph.p_filesz);

        m_file->seek(ph.p_offset, SEEK_SET);
        m_file->read(m_interpreter.data(), ph.p_filesz);
    }
}

bool ELF::load() {
    bool result = this->read_header();
    this->read_program_headers();

    return result;
}

}