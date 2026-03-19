#pragma once

#include <kernel/posix/sys/types.h>
#include <kernel/posix/time.h>

#define S_IFMT   0170000 /* type of file */

#define S_IFSOCK 0140000 /* socket */
#define S_IFLNK  0120000 /* symbolic link */
#define S_IFREG  0100000 /* regular */
#define S_IFBLK  0060000 /* block special */
#define S_IFDIR  0040000 /* directory */
#define S_IFCHR  0020000 /* character special */
#define S_IFIFO  0010000 /* fifo */

#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100
#define S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR)

#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010
#define S_IRWXG (S_IRGRP | S_IWGRP | S_IXGRP)

#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001
#define S_IRWXO (S_IROTH | S_IWOTH | S_IXOTH)

#define S_ISUID 0004000
#define S_ISGID 0002000
#define S_ISVTX 0001000

struct stat {
    dev_t      st_dev;          /* ID of device containing file */
    ino_t      st_ino;          /* Inode number */
    mode_t     st_mode;         /* File type and mode */
    nlink_t    st_nlink;        /* Number of hard links */
    uid_t      st_uid;          /* User ID of owner */
    gid_t      st_gid;          /* Group ID of owner */
    dev_t      st_rdev;         /* Device ID (if special file) */
    off_t      st_size;         /* Total size, in bytes */
    blksize_t  st_blksize;      /* Block size for filesystem I/O */
    blkcnt_t   st_blocks;       /* Number of 512 B blocks allocated */
    struct timespec  st_atim;   /* Time of last access */
    struct timespec  st_mtim;   /* Time of last modification */
    struct timespec  st_ctim;   /* Time of last status change */
};

#define st_atime  st_atim.tv_sec  /* Backward compatibility */
#define st_mtine  st_mtim.tv_sec
#define st_ctime  st_ctim.tv_sec
