#include <std/format.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

int main(int argc, char** argv) {
    auto* dir = opendir(argc > 1 ? argv[1] : ".");
    if (!dir) {
        dbgln("{}: {}: {}", argv[0], argv[1], strerror(errno));
        return 1;
    }

    for (;;) {
        struct dirent* entry = readdir(dir);
        if (!entry) {
            break;
        }
        
        String line;
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            line.append('/');
        }

        line.append(entry->d_name);
        dbgln(line);
    }

    closedir(dir);
    return 0;
}