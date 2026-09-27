#include <libgen.h>
#include <string.h>

extern "C" {

static char* __curr = const_cast<char*>(".");
static char* __root = const_cast<char*>("/");

char* dirname(char* path) {
    if (!path) {
        return __curr;
    }

    size_t len = strlen(path);
    if (!len) {
        return __curr;
    }

    while (len > 1 && path[len - 1] == '/') {
        len--;
        path[len] = '\0';
    }

    char* last_slash = strrchr(path, '/');
    if (!last_slash) {
        return __curr;
    }

    if (last_slash == path) {
        return __root;
    }

    *last_slash = '\0';
    return path;
}

char* basename(char* path) {
    if (!path) {
        return __curr;
    }

    size_t len = strlen(path);
    if (!len) {
        return __curr;
    }

    while (len > 1 && path[len - 1] == '/') {
        len--;
        path[len] = '\0';
    }

    char* last_slash = strrchr(path, '/');
    if (!last_slash) {
        return path;
    }

    if (len == 1) {
        return __root;
    }

    return last_slash + 1;
}

}