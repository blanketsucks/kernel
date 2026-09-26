#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include <std/function.h>
#include <std/format.h>

extern "C" {

static int __determine_argument_size(const char*& format) {
    size_t size = sizeof(int);
    switch (*format) {
        case 'z': {
            format++;
            size = sizeof(size_t);

            break;
        }
        case 'l': {
            format++;
            size = sizeof(long int);

            if (*format == 'l') {
                format++;
                size = sizeof(long long int);
            }

            break;
        }
        case 'h': {
            format++;
            size = sizeof(uint16_t);

            if (*format == 'h') {
                format++;
                size = sizeof(uint8_t);
            }

            break;
        }
        case 't': {
            format++;
            size = sizeof(ptrdiff_t);

            break;
        }
        case 'L': {
            format++;
            size = sizeof(long double);
        }
    }

    return size;
}

static void __store_num(void* dst, long long num, size_t size) {
    switch (size) {
        case 1: {
            *(u8*)dst = num; break; 
        }
        case 2: {
            *(u16*)dst = num; break;
        }
        case 4: {
            *(u32*)dst = num; break;
        }
        case 8: {
            *(u64*)dst = num; break;
        }
    }
}

static int __determine_number_sign(char& c, const Function<char()>& nextc) {
    if (c == '+') {
        c = nextc();
        return 1;
    } else if (c == '-') {
        c = nextc();
        return -1;
    } else {
        return 1;
    }
}

static int __scanf_internal(const Function<char()>& nextc, const char* format, va_list ap) {
    int assignments = 0;

    char buffer[64];

    char c = nextc();
    while (*format && c) {
        if (isspace(*format)) {
            while (isspace(c)) {
                c = nextc();
            }

            format++;
            continue;
        } else if (*format != '%') {
            if (*format++ != c) {
                return assignments;
            }

            c = nextc();
            continue;
        }

        format++;
        size_t width = 0;

        if (*format == '*') {
            width = va_arg(ap, int);
            format++;
        } else {
            while (isdigit(*format)) {
                width *= 10;
                width += *format - '0';

                format++;
            }
        }

        size_t size = __determine_argument_size(format);
        char spec = *format++;

        switch (spec) {
            case 'c': {
                char* out = reinterpret_cast<char*>(va_arg(ap, void*));
                width = width ? width : 1;

                while (width--) {
                    c = nextc();
                    *out++ = c;
                }

                assignments++;
                break;
            }
            case 'p':
            case 'x':
            case 'X': {
                int sign = __determine_number_sign(c, nextc);
                if (c == '0') {
                    c = nextc();
                    if (c == 'x' || c == 'X') {
                        c = nextc();
                    }
                }

                size_t n = 0;
                while (isxdigit(c)) {
                    buffer[n++] = c;
                    c = nextc();
                }

                buffer[n + 1] = 0;
                long long num = sign * strtoll(buffer, nullptr, 16);

                assignments++;
                __store_num(va_arg(ap, void*), num, size);

                break;
            }
            case 'd': {
                int sign = __determine_number_sign(c, nextc);

                size_t n = 0;
                while (isdigit(c)) {
                    buffer[n++] = c;
                    c = nextc();
                }

                buffer[n + 1] = 0;
                long long num = sign * strtoll(buffer, nullptr, 10);

                assignments++;
                __store_num(va_arg(ap, void*), num, size);

                break;
            }
            case 's': {
                if (!width) {
                    width = -1;
                }

                char* str = va_arg(ap, char*);
                while (!isspace(c) && c && width--) {
                    *str++ = c;
                    c = nextc();
                }

                *str = '\0';
                break;
            }
            default: {
                dbgln("UNSUPPORTED: scanf '{}' conversion spec.", spec);
            }
        }
    }

    return assignments;
}

int scanf(const char* format, ...) {
    va_list va;
    va_start(va, format);

    int result = vscanf(format, va);
    va_end(va);

    return result;
}

int fscanf(FILE* stream, const char* format, ...) {
    va_list va;
    va_start(va, format);

    int result = vfscanf(stream, format, va);
    va_end(va);

    return result;
}

int sscanf(const char* str, const char* format, ...) {
    va_list va;
    va_start(va, format);

    int result = vsscanf(str, format, va);
    va_end(va);

    return result;
}

int vscanf(const char* format, va_list ap) {
    return __scanf_internal([]() { return getchar(); }, format, ap);
}

int vfscanf(FILE* stream, const char* format, va_list ap) {
    return __scanf_internal([stream]() { return fgetc(stream); }, format, ap);
}

int vsscanf(const char* str, const char* format, va_list ap) {
    size_t index = 0;
    return __scanf_internal([&index, str]() {
        return str[index++];
    }, format, ap);
}

}