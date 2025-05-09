#pragma once

#include <sys/cdefs.h>
#include <sys/types.h>
#include <kernel/posix/sys/wait.h>

__BEGIN_DECLS

int waitpid(pid_t pid, int* status, int options);

__END_DECLS