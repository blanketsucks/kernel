#include <kernel/fs/ptsfs.h>
#include <std/hash_table.h>
#include <std/cstring.h>
#include <std/format.h>

namespace kernel::fs {

static HashTable<u32> s_ptys;

u32 PTSInode::major() const {
    return m_device_id.major;
}

u32 PTSInode::minor() const {
    return m_device_id.minor;
}

struct stat PTSInode::stat() const {
    // FIXME: Maybe return something useful here?
    return {};
}

Vector<DirectoryEntry> PTSInode::readdir() const {
    if (!this->is_directory()) {
        return {};
    }

    Vector<DirectoryEntry> entries;

    entries.append({ 1, DirectoryEntry::Directory, "." });

    // FIXME: Point `..` to `/dev`
    entries.append({ 1, DirectoryEntry::Directory, ".." });

    for (auto& pty : s_ptys) {
        entries.append({ pty + 2, DirectoryEntry::CharacterDevice, std::format("{}", pty) });
    }

    return entries;
}

RefPtr<Inode> PTSInode::lookup(StringView name) const {
    if (name == "." || name == "..") {
        return m_fs->inode(m_index);
    }

    u32 index = std::strntoul(name.data(), name.size(), nullptr, 10);
    if (index >= s_ptys.size()) {
        return nullptr;
    }

    return m_fs->inode(index + 2);
}

void PTSFS::init() {
    m_root = RefPtr<PTSInode>::make(this, DeviceID {}, 1);
}

RefPtr<Inode> PTSFS::inode(ino_t id) {
    if (id == 1) {
        return m_root;
    }

    auto* device = Device::get_device(169, id - 2);
    if (!device) {
        return nullptr;
    }

    return RefPtr<PTSInode>::make(this, device->id(), id - 2);
}

void PTSFS::register_pty(u32 pts) {
    s_ptys.set(pts);
}

void PTSFS::unregister_pty(u32 pts) {
    s_ptys.remove(pts);
}

}