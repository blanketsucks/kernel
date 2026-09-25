#pragma once

#include <sys/cdefs.h>
#include <stddef.h>

__BEGIN_DECLS

size_t wcslen(const wchar_t* s);
wchar_t* wcscat(wchar_t* dest, const wchar_t* src);

wchar_t* wmemcpy(wchar_t* dest, const wchar_t* src, size_t n);

__END_DECLS