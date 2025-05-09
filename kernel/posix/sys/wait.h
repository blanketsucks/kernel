#pragma once

#define __WIFEXITED 0x0100

#define WIFEXITED(w) ((w) & __WIFEXITED)
#define WEXITSTATUS(w) ((w) & 0x00ff)

#define WNOHANG 0x01