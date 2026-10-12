#include <strings.h>
#include <ctype.h>
#include <std/types.h>

extern "C" {

int strcasecmp(const char* s1, const char* s2) {
    const u8* p1 = (const u8*)s1;
    const u8* p2 = (const u8*)s2;

    while (*p1 && (tolower(*p1) == tolower(*p2))) {
        p1++;
        p2++;
    }

    return tolower(*p1) - tolower(*p2);
}

int strncasecmp(const char* s1, const char* s2, size_t n) {
    if (n == 0) {
        return 0;
    }

    const u8* p1 = (const u8*)s1;
    const u8* p2 = (const u8*)s2;

    while (n > 0) {
        int c1 = tolower(*p1);
        int c2 = tolower(*p2);

        if (c1 != c2) {
            return c1 - c2;
        } else if (*p1 == '\0') {
            break;
        }

        p1++; p2++;
        n--;
    }

    return 0;
}

}