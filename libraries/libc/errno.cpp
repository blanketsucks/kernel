#include <errno.h>
#include <sys/cdefs.h>

extern "C" {

int __errno;

int* __errno_location() {
    return &__errno;
}

}