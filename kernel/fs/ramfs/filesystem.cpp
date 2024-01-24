#if 0

#include <kernel/fs/ramfs/filesystem.h>
#include <kernel/fs/ramfs/file.h>
#include <kernel/vga.h>
#include <std/utility.h>

namespace kernel::ramfs {

FileSystem* FileSystem::s_instance = nullptr;

FileSystem::FileSystem() : m_root(new Node()) {
    m_root->name = "/";
    m_root->flags = Directory;
    m_root->data = nullptr;
    m_root->size = 0;
    m_root->parent = nullptr;
    m_root->id = nodes++;

    m_nodes.set(m_root->id, m_root);
    m_cwd = m_root;
}

FileSystem* FileSystem::create() {
    if (s_instance) return s_instance;

    s_instance = new FileSystem();
    return s_instance;
}

Node* FileSystem::node(ino_t id) const {
    return m_nodes.get(id).value_or(nullptr);
}

FileSystem* FileSystem::instance() {
    return FileSystem::s_instance;
}

void FileSystem::dump() const {
    this->dump(m_root, 0);
}

void FileSystem::dump(Node* node, int ident) const {
    for (int i = 0; i < ident; i++) {
        vga::printf(" ");
    }

    const char* type = nullptr;
    if (node->is_directory()) {
        type = "Directory";
    } else {
        type = "File";
    }

    vga::printf("- %*s", node->name.size(), node->name.data());
    if (node->children.empty()) {
        vga::printf(" (%s)\n", type);
    } else {
        vga::printf(" (%s):\n", type);

        for (auto& child : node->children) {
            this->dump(child, ident + 2);
        }
    }
}

ino_t FileSystem::resolve(StringView path) {
    if (path.empty()) {
        return 0;
    } else if (path == "/") {
        return m_root->id;
    }

    ino_t id = 0;
    if (path[0] == '/') {
        id = m_root->id;
        path = path.substr(1);
    }

    if (path.last() == '/') {
        path = path.substr(0, path.size() - 1);
    }

    while (path.size() > 0) {
        size_t slash = path.find('/');
        StringView name;

        if (slash == StringView::npos) {
            name = path;
            path = "";
        } else {
            name = path.substr(0, slash);
            path = path.substr(slash + 1);
        }

        if (name == ".") {
            continue;
        }

        if (name == "..") {
            Node* node = this->node(id);
            if (!node) return 0;

            id = node->parent->id;
            continue;
        }

        bool found = false;

        Node* parent = this->node(id);
        for (auto& child : parent->children) {
            if (child->name == name) {
                id = child->id;
                found = true;

                break;
            }
        }

        if (!found) return 0;
    }

    return id;
}

String FileSystem::fullpath(Node* node) const {
    if (node == m_root) {
        return "/";
    }

    Vector<Node*> nodes;
    while (node) {
        nodes.append(node);
        node = node->parent;
    }

    nodes.reverse();

    String path;
    for (auto& node : nodes) {
        path.append(node->name);
        path.append("/");
    }

    return path;
}

bool FileSystem::exists(StringView path) {
    return this->resolve(path) != 0;
}

bool FileSystem::is_directory(StringView path) {
    ino_t id = this->resolve(path);
    if (!id) return false;

    Node* node = this->node(id);
    return node->is_directory();
}

ErrorOr<size_t> FileSystem::read(Node* node, void* buffer, size_t size, size_t offset) const {
    if (node->is_directory()) {
        return Error(EISDIR);
    } else if (offset >= node->size) {
        return Error(EINVAL);
    }

    size_t read_size = std::min(size, node->size - offset);
    std::memcpy(buffer, (const char*)node->data + offset, read_size);

    return read_size;
}

ErrorOr<size_t> FileSystem::read(StringView path, void* buffer, size_t size, size_t offset) {
    ino_t id = this->resolve(path);
    if (!id) return Error(ENOENT);

    Node* node = this->node(id);
    return this->read(node, buffer, size, offset);
}

ErrorOr<size_t> FileSystem::write(Node* node, const void* buffer, size_t size, size_t offset) {
    if (node->is_directory()) {
        return Error(EISDIR);
    } else if (offset >= node->maxsize) {
        return Error(EINVAL);
    }

    while (offset + size > node->maxsize) {
        node->data = krealloc(node->data, node->maxsize * 2);
        node->maxsize *= 2;
    }

    size_t write_size = std::min(size, node->maxsize - offset);
    std::memcpy((char*)node->data + offset, buffer, write_size);

    node->size += write_size;
    return write_size;
}

ErrorOr<size_t> FileSystem::write(StringView path, const void* buffer, size_t size, size_t offset) {
    ino_t id = this->resolve(path);
    if (!id) return Error(ENOENT);

    Node* node = this->node(id);
    return this->write(node, buffer, size, offset);
}

ErrorOr<void> FileSystem::mkdir(StringView path, bool create_parents) {
    if (path.empty()) {
        return Error(ENOENT);
    }

    size_t position = path.find_last_of('/');
    Node* node = nullptr;

    if (position == StringView::npos) {
        node = m_cwd;
    } else if (position == 0) {
        node = m_root;
    } else {
        StringView p = path.substr(0, position);
        ino_t id = this->resolve(p);

        if (!id && create_parents) {
            this->mkdir(p, true);
            node = this->node(this->resolve(p));
        } else if (!node) {
            return Error(ENOENT);
        }
    }

    if (!node->is_directory()) {
        return Error(ENOTDIR);
    }

    Node* child = new Node();

    StringView name = path.substr(position + 1);
    if (name.last() == '/') {
        name = name.substr(0, name.size() - 1);
    }

    child->name = name;
    child->flags = Directory;
    child->data = nullptr;
    child->size = 0;
    child->parent = node;
    child->id = nodes++;

    node->children.append(child);
    return {};
}

ErrorOr<void> FileSystem::rmdir(StringView path) {
    ino_t id = this->resolve(path);
    if (!id) {
        return Error(ENOENT);
    }

    Node* node = this->node(id);
    if (!node->is_directory()) {
        return Error(ENOTDIR);
    }

    return this->remove(node);    
}

void FileSystem::chdir(Node* node) {
    m_cwd = node;
}

ErrorOr<void> FileSystem::chdir(StringView path) {
    ino_t id = this->resolve(path);
    if (!id) {
        return Error(ENOENT);
    }

    Node* node = this->node(id);
    if (!node->is_directory()) {
        return Error(ENOTDIR);
    }

    m_cwd = node;
    return {};
}

ErrorOr<OwnPtr<fs::File>> FileSystem::touch(StringView path) {
    if (path.empty()) {
        return Error(ENOENT);
    }

    size_t position = path.find_last_of('/');
    Node* node = nullptr;

    StringView name = path;
    if (position == StringView::npos) {
        node = m_cwd;
    } else if (position == 0) {
        node = m_root;
    } else {
        node = this->node(this->resolve(path.substr(0, position)));
        if (!node) {
            return Error(ENOENT);
        }

        name = path.substr(position + 1);
    }

    if (!node->is_directory()) {
        return Error(ENOTDIR);
    }
    
    Node* child = new Node();

    child->name = name;
    child->flags = 0;
    child->data = kmalloc(512);
    child->maxsize = 512;
    child->size = 0;
    child->parent = node;
    child->id = nodes++;

    node->children.append(child);
    return { OwnPtr<ramfs::File>::make(this, child) };
}

ErrorOr<void> FileSystem::rm(StringView path) {
    ino_t id = this->resolve(path);
    if (!id) {
        return Error(ENOENT);
    }

    Node* node = this->node(id);
    if (node->is_directory()) {
        return Error(EISDIR);
    }

    return this->remove(node);
}

ErrorOr<void> FileSystem::remove(Node* node) {
    if (node->children.size() > 0) {
        return Error(ENOTEMPTY);
    }

    node->parent->children.remove(node);
    delete node;

    return {};
}

struct stat FileSystem::stat(Node* node) const {
    // FIXME: Fill in the rest of the fields
    struct stat stat = {};

    stat.st_ino = node->id;
    stat.st_size = node->size;
    stat.st_blocks = node->maxsize / 512;
    stat.st_blksize = 512;

    mode_t mode = 0;
    if (node->is_directory()) {
        mode |= S_IFDIR;
    } else {
        mode |= S_IFREG;
    }

    stat.st_mode = mode;
    return stat;
}

struct stat FileSystem::stat(StringView path) {
    ino_t id = this->resolve(path);
    if (!id) {
        return {};
    }

    Node* node = this->node(id);
    return this->stat(node);
}

}

#endif