#pragma once

#include <kernel/posix/signal.h>
#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

int kill(pid_t pid, int sig);

typedef void (*sighandler_t)(int);

sighandler_t signal(int signum, sighandler_t handler);

__END_DECLS