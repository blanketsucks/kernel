#include <stdio.h>
#include <unistd.h>
#include <std/stb_sprintf.h>

extern "C" {

    
struct FILE {
    int fd;
};

FILE* stdin;
FILE* stdout;
FILE* stderr;

int dprintf(int fd, const char* format, ...) {
   va_list va;
   va_start(va, format);

   int result = vdprintf(fd, format, va);
   va_end(va);

   return result;
}

int sprintf(char* str, const char* format, ...) {
   va_list va;
   va_start(va, format);

   int result = vsprintf(str, format, va);
   va_end(va);

   return result;
}

int snprintf(char* str, size_t count, const char* format, ...) {
   va_list va;
   va_start(va, format);

   int result = vsnprintf(str, count, format, va);
   va_end(va);

   return result;
}

int vdprintf(int fd, const char *format, va_list ap) {
    char buffer[STB_SPRINTF_MIN];
    return stbsp_vsprintfcb([](const char* buffer, void* user, int len) -> char* {
        int fd = *reinterpret_cast<int*>(user);
        write(fd, buffer, len);

        return const_cast<char*>(buffer);
    }, &fd, buffer, format, ap);
}

int vsprintf(char* str, const char* format, va_list ap) {
    return stbsp_vsprintf(str, format, ap);
}

int vsnprintf(char* str, size_t size, const char* format, va_list ap) {
    return stbsp_vsnprintf(str, size, format, ap);
}

void __fseterr(FILE*) {}

}