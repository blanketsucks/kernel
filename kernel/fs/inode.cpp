#include <kernel/fs/inode.h>

namespace kernel::fs {

String ResolvedInode::fullpath() const {
    Vector<String> parts;
    for (auto* inode = this; inode != nullptr; inode = inode->parent()) {
        String name = inode->name();
        if (inode->parent() != nullptr && name.first() == '/') {
            name = name.substr(1);
        }

        parts.append(move(name));
    }

    parts.reverse();
    return String::join(parts, '/');
}

}