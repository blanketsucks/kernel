#pragma once

#include <sys/cdefs.h>
#include <stddef.h>
#include <stdarg.h>

#define _HAVE_STDIO

#ifndef EOF
    #define EOF (-1)
#endif

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

__BEGIN_DECLS

typedef struct FILE FILE;

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

int sprintf(char* str, const char* format, ...);

FILE* fopen(const char* pathname, const char* mode);
int fclose(FILE* stream);

size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);

int fseek(FILE* stream, long offset, int whence);
long ftell(FILE* stream);
int feof(FILE* stream);

int fflush(FILE* stream);

void setbuf(FILE* stream, char* buf);

int fprintf(FILE* stream, const char* format, ...);
int vfprintf(FILE* stream, const char* format, va_list ap);

__END_DECLS