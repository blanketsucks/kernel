#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <libelf/image.h>

// TODO: Implement
int main(int argc, char** argv) {
    if (argc < 2) {
        return EXIT_FAILURE;
    }

    int fd = open(argv[1], O_RDONLY);

    struct stat st;
    fstat(fd, &st);

    write(1, "Hello, World!\n", 14);

    void* buffer = mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    *(reinterpret_cast<u8*>(buffer) + 0x1200) = 0x42; 

    auto image = elf::Image(reinterpret_cast<u8*>(buffer), st.st_size);
    return 420;
}