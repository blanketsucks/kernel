#include <ctype.h>

extern "C" {

int isalnum(int c) {
    return isdigit(c) || isalpha(c);
}

int isalpha(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int isascii(int c) {
    return (unsigned)c <= 127;
}

int isdigit(int c) {
    return c >= '0' && c <= '9';
}

int islower(int c) {
    return c >= 'a' && c <= 'z';
}

int isupper(int c) {
    return c >= 'A' && c <= 'Z';
}

int toascii(int c) {
    return c & 127;
}

int tolower(int c) {
    return c >= 'A' && c <= 'Z' ? c | 0x20 : c;
}

int toupper(int c) {
    return c >= 'a' && c <= 'z' ? c & ~0x20 : c;
}

int isblank(int c) {
    return c == ' ' || c == '\t';
}

int iscntrl(int c) {
    return c <= 0x1F || c == 0x7F;
}

int isgraph(int c) {
    return c >= 33 && c <= 126;
}

int isprint(int c) {
    return c >= 32 && c <= 126;
}

int ispunct(int c) {
    return isprint(c) && !isspace(c) && !isalnum(c);
}

int isspace(int c) {
    return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
}

int isxdigit(int c) {
    return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

}