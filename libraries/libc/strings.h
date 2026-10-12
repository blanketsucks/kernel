#pragma once

#include <sys/cdefs.h>
#include <stddef.h>

__BEGIN_DECLS

int strcasecmp(const char* s1, const char* s2);
int strncasecmp(const char* s1, const char* s2, size_t n);

__END_DECLS