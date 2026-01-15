#include <stdlib.h>

extern "C" {

int main(int, char**, char**);

extern void _init();
extern char** environ;

static bool __called_constructors = false;
[[gnu::constructor]] void __dynamic_linker_init_check() {
    __called_constructors = true;
}

void _start(int argc, char** argv, char** envp) {
    environ = envp;

    if (!__called_constructors) {
        _init();
    }

    int status = main(argc, argv, envp);
    exit(status);
}

}