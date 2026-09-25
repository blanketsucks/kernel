#pragma once

typedef unsigned int rlim_t;

#define RLIM_INFINITY 0

#define RLIMIT_CORE 1
#define RLIMIT_CPU 2
#define RLIMIT_DATA 3
#define RLIMIT_FSIZE 4
#define RLIMIT_NOFILE 5
#define RLIMIT_STACK 6
#define RLIMIT_AS 7

#define RUSAGE_SELF 1
#define RUSAGE_CHILDREN 2