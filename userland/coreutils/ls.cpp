#include <std/format.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

#include <std/vector.h>

struct DirectoryEntry {
    String name;
    u8 type;
};

static void sort_entries(Vector<DirectoryEntry>& entries) {
    for (size_t i = 0; i < entries.size(); i++) {
        for (size_t j = i + 1; j < entries.size(); j++) {
            if (entries[i].type != DT_DIR && entries[j].type == DT_DIR) {
                std::swap(entries[j], entries[i]);
            }
        }
    }
}

int main(int argc, char** argv) {
    char cwd[256];
    getcwd(cwd, sizeof(cwd));

    auto* dir = opendir(argc > 1 ? argv[1] : cwd);
    if (!dir) {
        dbgln("{}: {}: {}", argv[0], argv[1], strerror(errno));
        return 1;
    }

    Vector<DirectoryEntry> entries;
    for (;;) {
        struct dirent* entry = readdir(dir);
        if (!entry) {
            break;
        }

        String name(entry->d_name);
        u8 type = entry->d_type;

        entries.append({ move(name), type });
    }

    sort_entries(entries);
    for (auto& entry : entries) {
        if (entry.type == DT_DIR && entry.name != "." && entry.name != "..") {
            entry.name.append('/');
        }

        dbgln("{}", entry.name);
    }

    closedir(dir);
    return 0;
}