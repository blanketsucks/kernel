#include <termios.h>
#include <errno.h>

extern "C" {

int tcgetattr(int, struct termios*) {
    errno = -ENOSYS;
    return -1;
}

}