#include <fcntl.h>
#include <sched.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <std/format.h>

[[gnu::constructor]] void before_main() {
    dbgln("Before main");
}

[[gnu::destructor]] void after_main() {
    dbgln("After main");
}

int main() {
    return 0;
}