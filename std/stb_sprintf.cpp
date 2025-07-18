#define STB_SPRINTF_IMPLEMENTATION

#ifdef __KERNEL__
    #define STB_SPRINTF_NOFLOAT
#endif

#include <std/stb_sprintf.h>