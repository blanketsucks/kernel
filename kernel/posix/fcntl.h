#pragma once

#define O_RDONLY    (1 << 0)
#define O_WRONLY    (1 << 1)
#define O_RDWR      (O_RDONLY | O_WRONLY)
#define O_CREAT     (1 << 2)
#define O_EXCL      (1 << 3)
#define O_TRUNC     (1 << 4)
#define O_APPEND    (1 << 5)
#define O_DIRECTORY (1 << 6)
#define O_NONBLOCK  (1 << 7)

#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4
#define F_DUPFD 5
#define F_DUPFD_CLOEXEC 6