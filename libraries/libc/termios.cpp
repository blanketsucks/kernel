#include <termios.h>
#include <errno.h>

extern "C" {

int tcgetattr(int, struct termios*) {
    errno = ENOSYS;
    return -1;
}

int tcsetattr(int, int, const struct termios*) {
    errno = ENOSYS;
    return -1;
}

speed_t cfgetospeed(const struct termios* term) {
    return term->c_ospeed;
}

speed_t cfgetispeed(const struct termios* term) {
    return term->c_ispeed;
}

int tcflush(int, int) {
    return 0;
}

}