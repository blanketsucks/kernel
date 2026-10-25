#pragma once

#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

#define KEY_MODIFIER_NONE  0 << 0
#define KEY_MODIFIER_SHIFT 1 << 0
#define KEY_MODIFIER_CTRL  1 << 1
#define KEY_MODIFIER_ALT   1 << 2

#define KEY_IS_PRESSED(x) ((x).scancode & 0x80) 

struct sys_keyboard_event {
    char ascii;
    uint8_t scancode;
    uint8_t modifiers;
} __attribute__((packed));

__END_DECLS