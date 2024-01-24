#include <kernel/fs/inode.h>

namespace kernel::fs {

String ResolvedInode::fullpath() const {
    Vector<String> parts;
    for (auto* inode = this; inode != nullptr; inode = inode->parent()) {
        parts.append(inode->name());
    }

    parts.reverse();
    return String::join(parts, '/');
}

}