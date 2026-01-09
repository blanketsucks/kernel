#include <kernel/process/process.h>

namespace kernel {

ErrorOr<FlatPtr> Process::sys$getcwd(char* buffer, size_t size) {
    this->validate_write(buffer, size);

    auto path = m_cwd->fullpath();
    if (path.size() > size) {
        return Error(ERANGE);
    }

    std::memcpy(buffer, path.data(), path.size());
    buffer[path.size()] = '\0';

    return 0;
}

ErrorOr<FlatPtr> Process::sys$chdir(const char* pathname) {
    StringView path = this->validate_string(pathname);

    auto vfs = fs::vfs();
    m_cwd = TRY(vfs->resolve(path, nullptr, m_cwd));

    return 0;
}

}