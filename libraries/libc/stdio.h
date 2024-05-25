#pragma once

#include <sys/cdefs.h>

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

__END_DECLS