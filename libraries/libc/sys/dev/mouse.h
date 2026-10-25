#pragma once

#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

#define MOUSE_BUTTON_LEFT   1 << 0
#define MOUSE_BUTTON_RIGHT  1 << 1
#define MOUSE_BUTTON_MIDDLE 1 << 2

struct sys_mouse_event {
    uint8_t x;
    uint8_t y;
    uint8_t z;
    uint8_t buttons;
} __attribute__((packed));

__END_DECLS