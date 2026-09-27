#include <locale.h>
#include <errno.h>

extern "C" {

char* setlocale(int, const char*) {
    errno = ENOSYS;
    return nullptr;
}

struct lconv* localeconv() {
    errno = ENOSYS;
    return nullptr;
}

}