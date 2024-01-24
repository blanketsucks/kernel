#pragma once

#include <kernel/common.h>

namespace kernel::serial {

bool init();
bool is_initialized();

bool is_transmit_empty();

void putc(char c);

void write(const char* str, size_t len);
void write(const char* str);

void printf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

}