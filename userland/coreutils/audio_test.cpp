#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <std/format.h>

int main() {
    int fd = open("/dev/snd0", O_WRONLY);
    if (fd < 0) {
        dbgln("Failed to open /dev/snd0");
        return 1;
    }

    int sample_rate, channels;

    ioctl(fd, SOUNDCARD_GET_SAMPLE_RATE, &sample_rate);
    ioctl(fd, SOUNDCARD_GET_CHANNELS, &channels);

    dbgln("Sample rate: {}", sample_rate);
    dbgln("Channels: {}", channels);

    int audio = open("/res/audio/audio.pcm", O_RDONLY);
    if (audio < 0) {
        dbgln("Failed to open /res/audio/audio.pcm");
        return 1;
    }

    struct stat st;
    fstat(audio, &st);

    char* buffer = new char[st.st_size];
    read(audio, buffer, st.st_size);

    write(fd, buffer, st.st_size);
    return 0;
}