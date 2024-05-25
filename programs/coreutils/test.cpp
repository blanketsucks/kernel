#include <unistd.h>
#include <fcntl.h>

int main() {
    int fd = open("/home/test.txt", O_RDWR, 0);

    char buffer[256];
    int nread = read(fd, buffer, sizeof(buffer));

    write(1, buffer, nread);
    close(fd);

    return 0;
}