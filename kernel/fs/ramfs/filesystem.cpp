#include <kernel/fs/ramfs/filesystem.h>

#include <kernel/posix/sys/stat.h>

namespace kernel::ramfs {

static ino_t s_next_id = 1;

RefPtr<Inode> Inode::create(String name, int flags, ino_t parent) {
    return RefPtr(new Inode(s_next_id++, move(name), flags, parent));
}

size_t Inode::read(void* buffer, size_t size, size_t offset) const {
    if (offset >= m_size) {
        return 0;
    } else if (offset + size > m_size) {
        size = m_size - offset;
    }

    std::memcpy(buffer, reinterpret_cast<u8*>(m_data) + offset, size);
    return size;
}

size_t Inode::write(const void* buffer, size_t size, size_t offset) {
    if (offset + size > m_size) {
        this->truncate(offset + size);
    }

    std::memcpy(reinterpret_cast<u8*>(m_data) + offset, buffer, size);
    return size;
}

void Inode::truncate(size_t size) {
    if (size == m_size) {
        return;
    }

    void* data = kmalloc(size);
    std::memcpy(data, m_data, size);

    kfree(m_data);

    m_data = data;
    m_size = size;
}

struct stat Inode::stat() const {
    struct stat stat = {};
    return stat;
}

Vector<fs::DirectoryEntry> Inode::readdir() const {
    if (!this->is_directory()) {
        return {};
    }

    Vector<fs::DirectoryEntry> entries;

    entries.append({ m_id, fs::DirectoryEntry::Directory, "." });
    entries.append({ m_parent, fs::DirectoryEntry::Directory, ".." });

    for (auto& [name, inode] : m_children) {
        auto type = inode->is_directory() ? fs::DirectoryEntry::Directory : fs::DirectoryEntry::RegularFile;
        entries.append({ inode->id(), type, name });
    }

    return entries;
}

RefPtr<fs::Inode> Inode::lookup(StringView name) const {
    auto it = m_children.find(name);
    if (it == m_children.end()) {
        return nullptr;
    }

    return it->value;
}

ErrorOr<void> Inode::add_entry(String name, RefPtr<fs::Inode> inode) {
    if (!this->is_directory()) {
        return Error(ENOTDIR);
    } else if (m_children.contains(name)) {
        return Error(EEXIST);
    }

    m_children.set(name, inode);
    return {};
}

// TODO: Handle mode (correctly), uid, gid
RefPtr<fs::Inode> Inode::create_entry(String name, mode_t mode, uid_t, gid_t) {
    if (!this->is_directory()) {
        return nullptr;
    }

    auto inode = Inode::create(name, mode, m_id);
    m_children.set(name, inode);

    return inode;
}

void Inode::flush() {}

FileSystem::FileSystem() {
    m_root = Inode::create("/", S_IFDIR | 0755, 0);
}

RefPtr<fs::Inode> FileSystem::inode(ino_t id) {
    if (id == this->root()) {
        return m_root;
    }

    return nullptr;
}

}