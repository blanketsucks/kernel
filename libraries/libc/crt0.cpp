
#include <stdlib.h>

extern "C" {

int main(int, char**, char**);

extern void _init();

extern char** environ;

void _start(int argc, char** argv, char** envp) {
    environ = envp;

    _init();

    int status = main(argc, argv, envp);
    exit(status);
}

}