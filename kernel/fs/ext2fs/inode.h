#pragma once

#include <kernel/fs/ext2fs/ext2.h>
#include <kernel/fs/file.h>
#include <kernel/fs/inode.h>

#include <kernel/common.h>

#include <std/vector.h>
#include <std/function.h>
#include <std/string.h>
#include <std/memory.h>
#include <std/result.h>
#include <kernel/posix/sys/stat.h>

namespace kernel::ext2fs {

class FileSystem;

class InodeEntry : public fs::Inode {
public:
    InodeEntry() = default;
    InodeEntry(FileSystem* fs, ext2fs::Inode inode, ino_t id);

    ext2fs::Inode const& inode() const { return m_inode; }

    Vector<u32> const& block_pointers() const { return m_block_pointers; }
    Vector<fs::DirectoryEntry> const& entries() const { return m_entries; }

    bool exists() const { return m_id != 0; }

    mode_t mode() const override { return m_inode.mode; }
    InodeType type() const { return m_inode.type(); }
    InodePermission permissions() const { return m_inode.permissions(); }

    bool is_fifo() const override { return m_inode.type() == InodeType::FIFO; }
    bool is_character_device() const override { return m_inode.type() == InodeType::CharacterDevice; }
    bool is_directory() const override { return m_inode.type() == InodeType::Directory; }
    bool is_block_device() const override { return m_inode.type() == InodeType::BlockDevice; }
    bool is_regular_file() const override { return m_inode.type() == InodeType::RegularFile; }
    bool is_symlink() const override { return m_inode.type() == InodeType::SymbolicLink; }
    bool is_unix_socket() const override { return m_inode.type() == InodeType::UnixSocket; }

    u32 major() const override { return m_device_major; }
    u32 minor() const override { return m_device_minor; }

    void set_device_major(u32 major) { m_device_major = major; }
    void set_device_minor(u32 minor) { m_device_minor = minor; }

    size_t size() const override { return m_inode.size_lower; }

    ssize_t read(void* buffer, size_t size, size_t offset) const override;
    ssize_t write(void const* buffer, size_t size, size_t offset) override;
    void truncate(size_t size) override;

    struct stat stat() const override;

    void readdir(std::Function<IterationAction(const fs::DirectoryEntry&)>) const override;
    RefPtr<fs::Inode> lookup(StringView name) const override;

    size_t block_count() const;
    u32 block_group_index() const;
    u32 block_group_offset() const;

    void read_blocks(size_t block, size_t count, u8* buffer) const;
    void write_blocks(size_t block, size_t count, u8 const* buffer);

    u32 get_block_pointer(size_t index) const;

    void read_block_pointers();
    ErrorOr<void> write_block_pointers();

    void read_singly_indirect_block_pointers(u32 block);
    void read_doubly_indirect_block_pointers(u32 block);
    void read_triply_indirect_block_pointers(u32 block);

    ErrorOr<void> write_singly_indirect_block_pointers(u32 block);
    ErrorOr<void> write_doubly_indirect_block_pointers(u32 block);
    ErrorOr<void> write_triply_indirect_block_pointers(u32 block);

    void set_disk_sectors();

    Vector<fs::DirectoryEntry> read_directory_entries() const;
    void write_directory_entries();

    ErrorOr<void> add_directory_entry(ino_t id, String name, fs::DirectoryEntry::Type type);

    ErrorOr<void> add_entry(String name, RefPtr<fs::Inode> inode) override;
    ErrorOr<void> remove_entry(StringView name) override;
    RefPtr<fs::Inode> create_entry(String name, mode_t mode, dev_t dev, uid_t uid, gid_t gid) override;

    void flush() override;

private:
    ext2fs::Inode m_inode;
    FileSystem* m_fs = nullptr;

    Vector<fs::DirectoryEntry> m_entries;

    Vector<u32> m_block_pointers;
    Vector<u32> m_indirect_block_pointers;

    u32 m_device_major = 0;
    u32 m_device_minor = 0;

    bool m_update_block_pointers = false;
};

}