#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#include <std/format.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        dbgln("Usage: {} <file>", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        dbgln("{}: {}: {}", argv[0], argv[1], strerror(errno));
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return 1;
    }

    size_t size = st.st_size;
    if (size == 0) {
        close(fd);
        return 0;
    }

    char* buffer = new char[4096];
    while (true) {
        ssize_t n = read(fd, buffer, 4096);
        write(1, buffer, n);

        if (n < 4096) {
            break;
        }
    }

    delete[] buffer;
    close(fd);
    
    return 0;
}