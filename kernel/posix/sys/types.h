#pragma once

#include <stdint.h>

/* There is no __SSIZE_TYPE__ but we can trick the preprocessor into defining it for us anyway! */
#define unsigned signed
typedef __SIZE_TYPE__ ssize_t;
#undef unsigned

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

typedef uint32_t uid_t;
typedef uint32_t gid_t;

typedef int __pid_t;
#define pid_t __pid_t

typedef char* caddr_t;

typedef int id_t;

typedef uint32_t ino_t;
typedef int32_t off_t;

typedef uint32_t blkcnt_t;
typedef uint32_t blksize_t;
typedef uint32_t dev_t;
typedef uint16_t mode_t;
typedef uint32_t nlink_t;

typedef int32_t time_t;
typedef uint32_t useconds_t;
typedef int32_t suseconds_t;
typedef uint64_t clock_t;

typedef uint32_t fsblkcnt_t;
typedef uint32_t fsfilcnt_t;