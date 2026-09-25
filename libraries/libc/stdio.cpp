#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <std/format.h>
// #include <std/stb_sprintf.h>

extern "C" {

static FILE* create_stream(int fd, int options, int mode = _IOLBF);
    
struct FILE {
    int fd;
    int options;
    int mode;

    char* buffer;
    off_t offset;
    size_t size;

    ssize_t err;

    bool is_unbuffered() const { return mode == _IONBF; }
    bool is_line_buffered() const { return mode == _IOLBF; }
};

FILE* stdin;
FILE* stdout;
FILE* stderr;

void __init_stdio() {
    stdin  = create_stream(0, O_RDONLY);
    stdout = create_stream(1, O_WRONLY);
    stderr = create_stream(2, O_WRONLY, _IONBF);
}

void __deinit_stdio() {
    // TODO: Close all the open files
    fclose(stdin);
    fclose(stdout);
    fclose(stderr);
}

static int parse_mode(const char* mode) {
    int options = 0;
    switch (mode[0]) {
        case 'r': options = O_RDONLY; break;
        case 'w': options = O_WRONLY | O_CREAT | O_TRUNC; break;
        case 'a': options = O_WRONLY | O_CREAT | O_APPEND; break;
    }

    if (mode[1] == '+') {
        options |= O_RDWR;
    }

    return options;
}

static FILE* create_stream(int fd, int options, int mode) {
    FILE* stream = new FILE;
    memset(stream, 0, sizeof(FILE));

    stream->fd = fd;
    stream->options = options;
    stream->mode = mode;

    setvbuf(stream, nullptr, mode, BUFSIZ);
    return stream;
}

FILE* fopen(const char* filename, const char* mode) {
    int options = parse_mode(mode);

    int fd = open(filename, options, 0666);
    if (fd < 0) {
        return nullptr;
    }

    return create_stream(fd, options);
}

FILE* fdopen(int fd, const char* mode) {
    int options = parse_mode(mode);
    return create_stream(fd, options);
}

int fclose(FILE* stream) {
    bool err = false;
    if (fflush(stream) < 0) {
        errno = stream->err; err = true;
    }

    if (close(stream->fd) < 0) {
        errno = stream->err; err = true;
    }

    if (stream->buffer) {
        free(stream->buffer);
    }

    free(stream);
    return err ? -1 : 0;
}

int setvbuf(FILE* stream, char* buf, int mode, size_t size) {
    if (mode != _IONBF && mode != _IOLBF && mode != _IOFBF) {
        return -1;
    }

    fflush(stream);
    if (stream->buffer) {
        free(stream->buffer);
    }

    if (mode == _IONBF) {
        stream->buffer = nullptr;
        stream->size = 0;

        return 0;
    }

    if (buf) {
        stream->buffer = buf;
    } else {
        stream->buffer = reinterpret_cast<char*>(malloc(size));
    }

    stream->size = size;
    return 0;
}

void setbuf(FILE* stream, char* buf) {
    setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
}

void setbuffer(FILE* stream, char* buf, size_t size) {
    setvbuf(stream, buf, buf ? _IOFBF : _IONBF, size);
}

void setlinebuf(FILE* stream) {
    setvbuf(stream, NULL, _IOLBF, 0);
}

int fileno(FILE* stream) {
    return stream->fd;
}

int fflush(FILE* stream) {
    if (stream->is_unbuffered()) {
        return 0;
    }

    if (stream->options & O_WRONLY && stream->buffer) {
        ssize_t n = write(stream->fd, stream->buffer, stream->offset);
        if (n < 0) {
            stream->err = errno;
        }
    }

    stream->offset    = 0;

    return 0;
}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) {
    size_t count = size * nmemb;
    if (stream->is_unbuffered()) {
        ssize_t n = write(stream->fd, ptr, count);
        if (n < 0) {
            stream->err = errno;
            return 0;
        }

        return n;
    }

    const char* buffer = reinterpret_cast<const char*>(ptr);
    size_t nwritten = 0;

    while (count) {
        size_t left = stream->size - stream->offset;
        if (count < left) {
            left = count;
        }

        if (!left) {
            fflush(stream);
            continue;
        }

        memcpy(stream->buffer + stream->offset, buffer, left);

        nwritten += left;
        count    -= left;
        
        stream->offset += left;
        if (stream->is_line_buffered() && memchr(buffer, '\n', left)) {
            fflush(stream);
        }

        buffer += left;
    }

    return nwritten / size;
}

int fprintf(FILE* stream, const char* format, ...) {
    va_list va;
    va_start(va, format);

    int result = vfprintf(stream, format, va);
    va_end(va);

    return result;
}

int printf(const char* format, ...) {
    va_list va;
    va_start(va, format);

    int result = vfprintf(stdout, format, va);
    va_end(va);

    return result;
}

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

int vfprintf(FILE* stream, const char* format, va_list ap) {
    char buffer[STB_SPRINTF_MIN];
    return stbsp_vsprintfcb([](const char* buffer, void* user, int len) -> char* {
        FILE* fp = reinterpret_cast<FILE*>(user);
        fwrite(buffer, len, 1, fp);

        return const_cast<char*>(buffer);
    }, stream, buffer, format, ap);
}

int fputc(int c, FILE *stream) {
    fwrite(&c, 1, 1, stream);
    return c;
}

int putc(int c, FILE *stream) {
    return fputc(c, stream);
}

int putchar(int c) {
    return fputc(c, stdout);
}

int fputs(const char* s, FILE* stream) {
    return fwrite(s, strlen(s), 1, stream);
}

int puts(const char* s) {
    fputs(s, stdout);
    fwrite("\n", 1, 1, stdout);

    return 0;
}

void __fseterr(FILE*) {}

}