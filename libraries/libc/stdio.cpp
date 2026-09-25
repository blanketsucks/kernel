#include <stdio.h>
#include <std/stb_sprintf.h>

extern "C" {

struct FILE {
    int fd;
};

int vsnprintf(char* str, size_t size, const char* format, va_list ap) {
    return stbsp_vsnprintf(str, size, format, ap);
}

void __fseterr(FILE*) {}

}