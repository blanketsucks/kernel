#include <kernel/fs/inode.h>
#include <std/format.h>

namespace kernel::fs {

String ResolvedInode::fullpath() const {
    // Only root doesn't have a parent
    if (!m_parent) {
        return "/";
    }

    Vector<String> parts;
    for (auto* inode = this; inode != nullptr; inode = inode->parent()) {
        parts.append(inode->name());
    }

    String fullpath;
    for (size_t i = parts.size(); i > 0; i--) {
        StringView path = parts[i - 1];

        bool has_leading_slash = path.startswith('/');
        bool has_trailing_slash = path.endswith('/');

        if (has_leading_slash && path != "/") {
            path = path.substr(1);
        } if (has_trailing_slash) {
            path = path.substr(0, path.size() - 1);
        }

        if (i != parts.size()) {
            fullpath.append('/');
        }

        fullpath.append(path);
    }

    return fullpath;
}

}