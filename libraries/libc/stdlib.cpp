#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <ctype.h>

#include <std/format.h>

extern "C" {

extern int __cxa_atexit(void (*func)(void*), void* arg, void* dso_handle);
extern void __cxa_finalize(void* dso_handle);
extern void _fini();

[[gnu::weak]] void __call_fini_functions() { _fini(); }

char** environ;

char *optarg;
int optind, opterr, optopt;

[[gnu::noreturn]] void exit(int status) {
    // TODO: Do extra cleanup tasks once we can actually do them.
    __cxa_finalize(nullptr);
    __call_fini_functions();

    _exit(status);
}

void abort(void) {
    // TODO: Send SIGABRT signal to the current process to terminate it.
    _exit(EXIT_FAILURE);
}

static void __atexit_wrapper(void* function) {
    reinterpret_cast<void(*)(void)>(function)();
}

int atexit(void (*function)(void)) {
    return __cxa_atexit(__atexit_wrapper, reinterpret_cast<void*>(function), nullptr);
}

int posix_openpt(int flags) {
    return open("/dev/ptmx", flags);
}

static char s_ptsname_buffer[64];
char* ptsname(int fd) {
    if (ptsname_r(fd, s_ptsname_buffer, sizeof(s_ptsname_buffer)) < 0) {
        return nullptr;
    }

    return s_ptsname_buffer;
}

int ptsname_r(int fd, char* buffer, size_t size) {
    int pts = -1;
    if (ioctl(fd, TIOCGPTN, &pts) < 0) {
        return -1;
    }

    std::FormatBuffer devfs;
    devfs.append("/dev/pts/");

    if (pts < 0) {
        errno = EINVAL;
        return -1;
    }

    devfs.appendf("%d", pts);
    if (devfs.position() > size) {
        errno = ERANGE;
        return -1;
    }

    StringView path = devfs.view();

    memset(buffer, 0, path.size() + 1);
    memcpy(buffer, path.data(), path.size());

    return 0;
}

int getopt(int argc, char *argv[], const char *optstring) {
    return -ENOSYS;
}

char* getenv(const char* name) {
    size_t len = strlen(name);
    for (size_t i = 0; environ[i]; i++) {
        char* var = environ[i];
        char* eq = strchr(var, '=');

        if (!eq) {
            continue;
        }

        size_t value_len = eq - var;
        if (len != value_len) {
            continue;
        }

        if (strncmp(var, name, value_len) == 0) {
            return eq + 1;
        }
    }

    return nullptr;
} 

int setenv(const char* name, const char* value, int overwrite) {
    return -1;
}

int unsetenv(const char* name) {
    return -1;
}

int putenv(char* string) {
    return -1;
}

bool is_valid_digit(char digit, int base) {
    if (digit < '0') {
        return false;
    }

    if (base <= 10) {
        return digit <= ('0' + base - 1);
    }

    if (digit >= '0' && digit <= '9') {
        return true;
    } else if (digit >= 'A' && digit < 'A' + (base - 10)) {
        return true;
    } else if (digit >= 'a' && digit < 'a' + (base - 10)) {
        return true;
    }

    return false;
}

static int parse_digit(char digit) {
    if (digit >= '0' && digit <= '9') {
        return digit - '0';
    } else if (digit >= 'a' && digit <= 'z') {
        return digit - 'a' + 0xa;
    } else if (digit >= 'A' && digit <= 'Z') {
        return digit - 'A' + 0xa;
    }

    return -1;
}

long long int strtoll(const char* nptr, char** endptr, int base) {
    if (base < 0 || base > 36 || base == 1) {
        errno = EINVAL;
        return INT64_MAX;
    }

    const char* ch = nptr;
    while (*ch && isspace(*ch)) {
        ch++;
    }

    int sign = 1;
    if(*ch == '-') {
        sign = -1;
        ch++;
    } else if (*ch == '+') {
        ch++;
    }

    if (base == 0) {
        if (*ch == '0') {
            if (tolower(*(ch + 1)) == 'x') {
                ch += 2;
                base = 16;
            } else {
                ch++;
                base = 8;
            }
        } else {
            base = 10;
        }
    }

    if(base == 16 && *ch == '0') {
        ch++;
        if (tolower(*ch) == 'x') {
            ch++;
        }
    }

    long long int val = 0;
    int overflow = 0;

    while (is_valid_digit(*ch, base)) {
        val *= base;
        val += parse_digit(*ch++);

        if (val < 0) {
            overflow = 1;
        }
    }

    if (endptr) {
        *endptr = (char*)ch;
    }

    if (overflow) {
        errno = ERANGE;
        if (sign == 1) {
            return INT64_MAX;
        } else {
            return INT64_MIN;
        }
    }

    return sign == -1 ? -val : val;
}

long strtol(const char* nptr, char** endptr, int base) {
	long long int val = strtoll(nptr, endptr, base);
	if (val > INT64_MAX) {
		errno = ERANGE;
		return INT64_MAX;
	} else if (val < INT64_MIN) {
		errno = ERANGE;
		return INT64_MIN;
	}

	return (long)val;
}

int atoi(const char* nptr) {
    long value = strtol(nptr, nullptr, 10);
    if (value > INT32_MAX) {
        return INT32_MAX;
    }

    return value;
}
    
}