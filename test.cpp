#include <stddef.h>

#define SYS_EXIT 0
#define SYS_OPEN 1
#define SYS_CLOSE 2
#define SYS_READ 3
#define SYS_WRITE 4

#define O_RDONLY (1 << 0)
#define O_WRONLY (1 << 1)
#define O_RDWR   (O_RDONLY | O_WRONLY)

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }

    return len;
}

int syscall(int n, int ebx = 0, int ecx = 0, int edx = 0, int esi = 0, int edi = 0) {
    int result = 0;
    asm volatile("int $0x80" : "=a"(result) : "a"(n), "b"(ebx), "c"(ecx), "d"(edx), "S"(esi), "D"(edi));

    return result;
}

void exit(int status) {
    syscall(SYS_EXIT, status);
}

int open(const char* path, int flags, int mode) {
    return syscall(SYS_OPEN, reinterpret_cast<int>(path), strlen(path), flags, mode);
}

int close(int fd) {
    return syscall(SYS_CLOSE, fd);
}

int read(int fd, void* buffer, size_t size) {
    return syscall(SYS_READ, fd, reinterpret_cast<int>(buffer), size);
}

int write(int fd, const void* buffer, size_t size) {
    return syscall(SYS_WRITE, fd, reinterpret_cast<int>(buffer), size);
}

extern "C" void _start() {
    int fd = open("/home/test.txt", O_RDWR, 0);

    char buffer[256];
    read(fd, buffer, sizeof(buffer));

    write(1, buffer, sizeof(buffer));
    close(fd);

    exit(0);
}