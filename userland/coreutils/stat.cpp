#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <std/format.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        dbgln("Usage: {} <file>", argv[0]);
        return EXIT_FAILURE;
    }

    struct stat st;
    if (stat(argv[1], &st) < 0) {
        dbgln("Failed to stat file '{}': {}", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }

    dbgln("Stat for file '{}':", argv[1]);
    dbgln("  Mode: {:#x}", st.st_mode);
    dbgln("  Size: {}", st.st_size);
    dbgln("  Inode: {}", st.st_ino);
    dbgln("  Block size: {}", st.st_blksize);
    dbgln("  Block count: {}", st.st_blocks);

    return 0;
}
