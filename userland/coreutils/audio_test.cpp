#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include <std/format.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        dbgln("Usage: {} <file>", argv[0]);
        return 1;
    }

    int fd = open("/dev/snd0", O_WRONLY);
    if (fd < 0) {
        dbgln("Failed to open /dev/snd0: {}", strerror(errno));
        return 1;
    }

    int sample_rate, channels;

    ioctl(fd, SOUNDCARD_GET_SAMPLE_RATE, &sample_rate);
    ioctl(fd, SOUNDCARD_GET_CHANNELS, &channels);

    dbgln("Sample rate: {}", sample_rate);
    dbgln("Channels: {}", channels);

    int audio = open(argv[1], O_RDONLY);
    if (audio < 0) {
        dbgln("Failed to open {}: {}", argv[1], strerror(errno));
        return 1;
    }

    struct stat st;
    fstat(audio, &st);

    char* buffer = new char[st.st_size];
    read(audio, buffer, st.st_size);

    write(fd, buffer, st.st_size);

    close(audio);
    close(fd);

    delete[] buffer;

    return 0;
}