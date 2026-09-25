#pragma once

#include <stddef.h>

#define SIGABRT 1
#define SIGINT 2
#define SIGTERM 3

#define SIG_DFL (void*)0
#define SIG_ERR (void*)0
#define SIG_IGN (void*)0

#define SIG_ATOMIC_T sig_atomic_t
typedef uint32_t sig_atomic_t;