#pragma once

#include <sys/cdefs.h>
#include <stddef.h>
#include <stdarg.h>
#include <assert.h>

#define _HAVE_STDIO

#ifndef EOF
    #define EOF (-1)
#endif

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define _IOFBF 0 // Fully buffered
#define _IOLBF 1 // Line buffered
#define _IONBF 2 // Unbuffered

#define BUFSIZ 4096

__BEGIN_DECLS

typedef struct FILE FILE;

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

FILE* fopen(const char* pathname, const char* mode);
FILE* fdopen(int fd, const char* mode);
int fclose(FILE* stream);

size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);

int fseek(FILE* stream, long offset, int whence);
long ftell(FILE* stream);
void rewind(FILE *stream);
int feof(FILE* stream);

int fflush(FILE* stream);
int fileno(FILE* stream);

void clearerr(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);

int fputc(int c, FILE *stream);
int putc(int c, FILE *stream);
int putchar(int c);

char* fgets(char* s, int size, FILE* stream);

int fputs(const char* s, FILE* stream);
int puts(const char* s);

int fgetc(FILE *stream);
int getc(FILE *stream);
int getchar(void);

int setvbuf(FILE* stream, char* buf, int mode, size_t size);
void setbuf(FILE* stream, char* buf);
void setbuffer(FILE* stream, char* buf, size_t size);
void setlinebuf(FILE* stream);

int printf(const char* format, ...) __attribute__((format(printf, 1, 2)));
int fprintf(FILE* stream, const char* format, ...) __attribute__((format(printf, 2, 3)));
int dprintf(int fd, const char* format, ...) __attribute__((format(printf, 2, 3)));
int sprintf(char* str, const char* format, ...) __attribute__((format(printf, 2, 3)));
int snprintf(char* str, size_t size, const char* format, ...) __attribute__((format(printf, 3, 4)));

int vprintf(const char* format, va_list ap);
int vfprintf(FILE* stream, const char* format, va_list ap);
int vdprintf(int fd, const char* format, va_list ap);
int vsprintf(char* str, const char* format, va_list ap);
int vsnprintf(char* str, size_t size, const char* format, va_list ap);

int scanf(const char* format, ...) __attribute__((format(scanf, 1, 2)));
int fscanf(FILE* stream, const char* format, ...) __attribute__((format(scanf, 2, 3)));
int sscanf(const char* str, const char* format, ...) __attribute__((format(scanf, 2, 3)));

int vscanf(const char* format, va_list ap);
int vfscanf(FILE* stream, const char* format, va_list ap);
int vsscanf(const char* str, const char* format, va_list ap);

char* tmpnam(char* s);

void __fseterr(FILE* stream);

void __init_stdio();
void __deinit_stdio();

__END_DECLS