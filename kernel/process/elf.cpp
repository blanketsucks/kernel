#include <kernel/process/elf.h>
#include <kernel/posix/unistd.h>
#include <kernel/fs/vfs.h>

#include <std/format.h>

namespace kernel {

ErrorOr<RefPtr<ELF>> ELF::create(RefPtr<fs::FileDescriptor> file) {
    auto elf = RefPtr<ELF>(new ELF(file));
    TRY(elf->load());

    return elf;
}

ErrorOr<void> ELF::read_header() {
    if (m_header) {
        return {};
    }

    auto* header = m_header = new ELFHeader;

    m_file->seek(0, SEEK_SET);
    TRY(m_file->read(header, sizeof(ELFHeader)));

    u32 magic = *reinterpret_cast<u32*>(header);
    if (magic != ELF_MAGIC) {
        return Error(EINVAL);
    } else if (header->e_ident[EI_CLASS] != ELFCLASS64 && header->e_ident[EI_CLASS] != ELFCLASS32) {
        return Error(ENOTSUP);
    }

    return {};
}

ErrorOr<void> ELF::read_program_headers() {
    if (!m_header || !m_program_headers.empty()) {
        return {};
    }

    m_program_headers.resize(m_header->e_phnum);

    m_file->seek(m_header->e_phoff, SEEK_SET);
    TRY(m_file->read(m_program_headers.data(), m_header->e_phentsize * m_header->e_phnum));

    for (auto& ph : m_program_headers) {
        if (ph.p_type != PT_INTERP) {
            continue;
        }

        m_interpreter.resize(ph.p_filesz - 1);

        m_file->seek(ph.p_offset, SEEK_SET);
        TRY(m_file->read(m_interpreter.data(), ph.p_filesz - 1));
    }

    return {};
}

ErrorOr<void> ELF::load() {
    TRY(this->read_header());
    TRY(this->read_program_headers());

    return {};
}

}