#if 0

#include <kernel/common.h>
#include <std/string.h>
#include <std/hash_map.h>
#include <std/memory.h>

#include <kernel/fs/filesystem.h>
#include <kernel/fs/file.h>

namespace kernel::ramfs {

enum NodeFlags {
    Directory = 1 << 0,
};

struct Node {
    String name;
    int flags;

    void* data;

    size_t maxsize; // The size of the allocated buffer
    size_t size;    // The size of the data stored in the buffer

    ino_t id;

    Node* parent;
    Vector<Node*> children;

    bool is_root() const { return parent == nullptr; }
    bool is_directory() const { return flags & Directory; }
};

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

}

#endif