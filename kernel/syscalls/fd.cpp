#include <kernel/process/process.h>

namespace kernel {

ErrorOr<FlatPtr> Process::sys$open(const char* pathname, size_t pathname_length, int flags, mode_t mode) {
    StringView path = this->validate_string(pathname, pathname_length);
    auto vfs = fs::vfs();

    auto fd = TRY(vfs->open(path, flags, mode, m_cwd));

    m_file_descriptors.append(move(fd));
    return m_file_descriptors.size() - 1;
}

ErrorOr<FlatPtr> Process::sys$close(int fd) {
    if (fd < 0 || static_cast<size_t>(fd) >= m_file_descriptors.size()) {
        return Error(EBADF);
    }

    auto& file = m_file_descriptors[fd];
    if (file) {
        file.~RefPtr();
    }

    m_file_descriptors[fd] = nullptr;
    return 0;
}

ErrorOr<FlatPtr> Process::sys$read(int fd, void* buffer, size_t size) {
    auto file = this->get_file_descriptor(fd);
    if (!file) {
        return Error(EBADF);
    }

    this->validate_read(buffer, size);
    return file->read(buffer, size);
}

ErrorOr<FlatPtr> Process::sys$write(int fd, const void* buffer, size_t size) {
    auto file = this->get_file_descriptor(fd);
    if (!file) {
        return Error(EBADF);
    }

    this->validate_read(buffer, size);
    return file->write(buffer, size);
}

ErrorOr<FlatPtr> Process::sys$stat(const char* path, size_t path_length, stat* buffer) {
    StringView pathname = this->validate_string(path, path_length);
    auto vfs = fs::vfs();

    auto resolved = TRY(vfs->resolve(pathname, nullptr, m_cwd));
    this->validate_write(buffer, sizeof(stat));

    *buffer = resolved->inode().stat();
    return 0;
}

ErrorOr<FlatPtr> Process::sys$fstat(int fd, stat* buffer) {
    auto file = this->get_file_descriptor(fd);

    this->validate_write(buffer, sizeof(stat));
    *buffer = file->stat();

    return 0;
}

ErrorOr<FlatPtr> Process::sys$lseek(int fd, off_t offset, int whence) {
    auto file = this->get_file_descriptor(fd);
    if (!file) {
        return Error(EBADF);
    }

    file->seek(offset, whence);
    return file->offset();
}

ErrorOr<FlatPtr> Process::sys$dup(int old_fd) {
    auto file = this->get_file_descriptor(old_fd);
    if (!file) {
        return Error(EBADF);
    }

    int new_fd = -1;
    for (size_t i = 0; i < m_file_descriptors.size(); i++) {
        if (!m_file_descriptors[i]) {
            new_fd = i;
            break;
        }
    }

    if (new_fd == -1) {
        m_file_descriptors.append(file);
        new_fd = m_file_descriptors.size() - 1;
    } else {
        m_file_descriptors[new_fd] = file;
    }

    return new_fd;
}

ErrorOr<FlatPtr> Process::sys$dup2(int old_fd, int new_fd) {
    auto file = this->get_file_descriptor(old_fd);

    if (new_fd < 0 || static_cast<size_t>(new_fd) >= m_file_descriptors.size()) {
        return Error(EBADF);
    }

    m_file_descriptors[new_fd] = file;
    return new_fd;
}

ErrorOr<FlatPtr> Process::sys$readdir(int fd, void* buffer, size_t size) {
    auto file = this->get_file_descriptor(fd);
    if (!file) {
        return Error(EBADF);
    }

    return file->file()->readdir(buffer, size);
}

ErrorOr<FlatPtr> Process::sys$ioctl(int fd, unsigned request, unsigned arg) {
    auto file = this->get_file_descriptor(fd);
    if (!file) {
        return Error(EBADF);
    }

    return TRY(file->ioctl(request, arg));
}

}