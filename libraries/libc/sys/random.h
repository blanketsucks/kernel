#pragma once

#include <sys/cdefs.h>
#include <sys/types.h>
#include <stddef.h>

__BEGIN_DECLS

ssize_t getrandom(void* buf, size_t buflen, unsigned int flags);

__END_DECLS