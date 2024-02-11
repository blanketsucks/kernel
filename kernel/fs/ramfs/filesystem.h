#pragma once

#include <kernel/common.h>
#include <std/string.h>
#include <std/hash_map.h>
#include <std/memory.h>

#include <kernel/fs/filesystem.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/file.h>

namespace kernel::ramfs {

constexpr ino_t ROOT_INODE = 1;

enum NodeFlags {
    Directory = 1 << 0,
};

struct Inode : public fs::Inode {
public:
    virtual ~Inode() = default;

    size_t read(void* buffer, size_t size, size_t offset) const override;
    size_t write(const void* buffer, size_t size, size_t offset) override;
    void truncate(size_t size) override;

    mode_t mode() const override;
    size_t size() const override { return m_size; }

    bool is_fifo() const override;
    bool is_character_device() const override;
    bool is_directory() const override;
    bool is_block_device() const override;
    bool is_regular_file() const override;
    bool is_symlink() const override;
    bool is_unix_socket() const override;

    u32 major() const override { return 0; }
    u32 minor() const override { return 0; }

    struct stat stat() const override;

    Vector<fs::DirectoryEntry> readdir() const override;
    RefPtr<fs::Inode> lookup(StringView name) const override;

    void add_entry(String name, RefPtr<fs::Inode> inode) override;
    RefPtr<fs::Inode> create_entry(String name, mode_t mode, uid_t uid, gid_t gid) override;

    void flush() override;

private:
    String m_name;
    int m_flags;

    void* m_data;

    size_t m_maxsize; // The size of the allocated buffer
    size_t m_size;    // The size of the data stored in the buffer

    ino_t m_id;

    Inode* m_parent;
    Vector<Inode*> m_children;
};

class FileSystem : public fs::FileSystem {
public:
    u8 id() const override { return 0x01; }
    StringView name() const override { return "ramfs"; }

    ino_t root() const override { return ROOT_INODE; }
    RefPtr<fs::Inode> inode(ino_t id) override;

private:
    Inode* m_root;
};

#if 0

class FileSystem : public fs::FileSystem {
public:
    static FileSystem* create();

    bool exists(StringView path) override;
    bool is_directory(StringView path) override;

    ErrorOr<size_t> read(StringView path, void* buffer, size_t size, size_t offset = 0) override;
    ErrorOr<size_t> write(StringView path, const void* buffer, size_t size, size_t offset = 0) override;

    ErrorOr<size_t> read(Node* node, void* buffer, size_t size, size_t offset = 0) const;
    ErrorOr<size_t> write(Node* node, const void* buffer, size_t size, size_t offset = 0);

    ErrorOr<void> mkdir(StringView path, bool create_parents = false) override;
    ErrorOr<void> rmdir(StringView path) override;

    ino_t resolve(StringView path) override;

    void chdir(Node* node);
    ErrorOr<void> chdir(StringView path);

    ErrorOr<OwnPtr<fs::File>> touch(StringView path) override;
    ErrorOr<void> rm(StringView path) override;

    struct stat stat(StringView path) override;
    struct stat stat(Node* node) const;

    Node* node(ino_t id) const;

    ErrorOr<void> remove(Node* node); // A generic remove function that can be used for both files and directories

    void dump() const;
    void dump(Node* node, int indent = 0) const;

    Node* root() const { return m_root; }
    Node* cwd() const { return m_cwd; }

    String fullpath(Node* node) const;

private:
    FileSystem();

    Node* m_root;
    Node* m_cwd; // TODO: This would be process-specific once we actually have processes

    HashMap<ino_t, Node*> m_nodes;

    ino_t nodes = 1; // The next node ID to be assigned
};

#endif

}
