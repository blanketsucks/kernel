#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <std/format.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        dbgln("Usage: {} <directory>", argv[0]);
        return EXIT_FAILURE;
    }

    if (mkdir(argv[1], S_IRWXU) < 0) {
        dbgln("Failed to create directory '{}': {}", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}