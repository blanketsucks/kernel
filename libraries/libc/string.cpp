#include <string.h>
#include <errno.h>
#include <std/types.h>

extern "C" {

const char* errno_descriptions[] = {
    "Success",
    "Operation not permitted",               // EPERM
    "No such file or directory",             // ENOENT
    "No such process",                       // ESRCH
    "Interrupted system call",               // EINTR
    "Input/output error",                    // EIO
    "No such device or address",             // ENXIO
    "Argument list too long",                // E2BIG
    "Exec format error",                     // ENOEXEC
    "Bad file descriptor",                   // EBADF
    "No child processes",                    // ECHILD
    "Resource temporarily unavailable",      // EAGAIN
    "Cannot allocate memory",                // ENOMEM
    "Permission denied",                     // EACCES
    "Bad address",                           // EFAULT
    "Block device required",                 // ENOTBLK
    "Device or resource busy",               // EBUSY
    "File exists",                           // EEXIST
    "Invalid cross-device link",             // EXDEV
    "No such device",                        // ENODEV
    "Not a directory",                       // ENOTDIR
    "Is a directory",                        // EISDIR
    "Invalid argument",                      // EINVAL
    "Too many open files in system",         // ENFILE
    "Too many open files",                   // EMFILE
    "Inappropriate ioctl for device",        // ENOTTY
    "Text file busy",                        // ETXTBSY
    "File too large",                        // EFBIG
    "No space left on device",               // ENOSPC
    "Illegal seek",                          // ESPIPE
    "Read-only file system",                 // EROFS
    "Too many links",                        // EMLINK
    "Broken pipe",                           // EPIPE
    "Numerical argument out of domain",      // EDOM
    "Numerical result out of range",         // ERANGE
    "Resource deadlock avoided",             // EDEADLK
    "File name too long",                    // ENAMETOOLONG
    "No locks available",                    // ENOLCK
    "Function not implemented",              // ENOSYS
    "Directory not empty",                   // ENOTEMPTY
    "Too many levels of symbolic links",     // ELOOP
    "Unknown error 41",
    "No message of desired type",            // ENOMSG
    "Identifier removed",                    // EIDRM
    "Channel number out of range",           // ECHRNG
    "Level 2 not synchronized",              // EL2NSYNC
    "Level 3 halted",                        // EL3HLT
    "Level 3 reset",                         // EL3RST
    "Link number out of range",              // ELNRNG
    "Protocol driver not attached",          // EUNATCH
    "No CSI structure available",            // ENOCSI
    "Level 2 halted",                        // EL2HLT
    "Invalid exchange",                      // EBADE
    "Invalid request descriptor",            // EBADR
    "Exchange full",                         // EXFULL
    "No anode",                              // ENOANO
    "Invalid request code",                  // EBADRQC
    "Invalid slot",                          // EBADSLT
    "Unknown error 58",
    "Bad font file format",                  // EBFONT
    "Device not a stream",                   // ENOSTR
    "No data available",                     // ENODATA
    "Timer expired",                         // ETIME
    "Out of streams resources",              // ENOSR
    "Machine is not on the network",         // ENONET
    "Package not installed",                 // ENOPKG
    "Object is remote",                      // EREMOTE
    "Link has been severed",                 // ENOLINK
    "Advertise error",                       // EADV
    "Srmount error",                         // ESRMNT
    "Communication error on send",           // ECOMM
    "Protocol error",                        // EPROTO
    "Multihop attempted",                    // EMULTIHOP
    "RFS specific error",                    // EDOTDOT
    "Bad message",                           // EBADMSG
    "Value too large for defined data type", // EOVERFLOW
    "Name not unique on network",            // ENOTUNIQ
    "File descriptor in bad state",          // EBADFD
    "Remote address changed",                // EREMCHG
    "Can not access a needed shared library", // ELIBACC
    "Accessing a corrupted shared library",  // ELIBBAD
    "lib section in a.out corrupted",        // ELIBSCN
    "Attempting to link in too many shared libraries", // ELIBMAX
    "Cannot exec a shared library directly", // ELIBEXEC
    "Invalid or incomplete multibyte or wide character", // EILSEQ
    "Interrupted system call should be restarted", // ERESTART
    "Streams pipe error",                    // ESTRPIPE
    "Too many users",                        // EUSERS
    "Socket operation on non-socket",        // ENOTSOCK
    "Destination address required",          // EDESTADDRREQ
    "Message too long",                      // EMSGSIZE
    "Protocol wrong type for socket",        // EPROTOTYPE
    "Protocol not available",                // ENOPROTOOPT
    "Protocol not supported",                // EPROTONOSUPPORT
    "Socket type not supported",             // ESOCKTNOSUPPORT
    "Operation not supported",               // EOPNOTSUPP/ENOTSUP
    "Protocol family not supported",         // EPFNOSUPPORT
    "Address family not supported by protocol", // EAFNOSUPPORT
    "Address already in use",                // EADDRINUSE
    "Cannot assign requested address",       // EADDRNOTAVAIL
    "Network is down",                       // ENETDOWN
    "Network is unreachable",                // ENETUNREACH
    "Network dropped connection on reset",   // ENETRESET
    "Software caused connection abort",      // ECONNABORTED
    "Connection reset by peer",              // ECONNRESET
    "No buffer space available",             // ENOBUFS
    "Transport endpoint is already connected", // EISCONN
    "Transport endpoint is not connected",   // ENOTCONN
    "Cannot send after transport endpoint shutdown", // ESHUTDOWN
    "Too many references: cannot splice",    // ETOOMANYREFS
    "Connection timed out",                  // ETIMEDOUT
    "Connection refused",                    // ECONNREFUSED
    "Host is down",                          // EHOSTDOWN
    "No route to host",                      // EHOSTUNREACH
    "Operation already in progress",         // EALREADY
    "Operation now in progress",             // EINPROGRESS
    "Stale file handle",                     // ESTALE
};

size_t strlen(const char* s) {
    size_t len = 0;
    while (*(s++)) {
        ++len;
    }

    return len;
}

int strcmp(const char* str1, const char* str2) {
    size_t i = 0;
    while (str1[i] && str2[i] && str1[i] == str2[i]) {
        i++;
    }

    return str1[i] - str2[i];
}

int strncmp(const char* str1, const char* str2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (str1[i] != str2[i]) {
            return str1[i] - str2[i];
        }
    }

    return 0;
}

void* memset(void* dest, int c, size_t n) {
    asm volatile("rep stosb" : "+D"(dest), "+c"(n) : "a"(c) : "memory");
    return dest;
}

void* memcpy(void* dest, const void* src, size_t n) {
    asm volatile("rep movsb" : "+D"(dest), "+S"(src), "+c"(n) : : "memory");
    return dest;
}

int memcmp(const void* ptr1, const void* ptr2, size_t n) {
    auto* s1 = static_cast<const u8*>(ptr1);
    auto* s2 = static_cast<const u8*>(ptr2);

    while (n-- > 0) {
        if (*s1++ != *s2++) {
            return s1[-1] < s2[-1] ? -1 : 1;
        }
    }
    return 0;
}

void* memmove(void* dest, void const* src, size_t n) {
    if (dest < src) {
        return memcpy(dest, src, n);
    }

    u8* d = (u8*)dest;
    u8 const* s = (u8 const*)src;

    for (d += n, s += n; n--;) {
        *--d = *--s;
    }

    return dest;
}

char* strerror(int errnum) {
    if (errnum < 0 || errnum >= EMAXERRNO) {
        return const_cast<char*>("Unkown error");
    }

    return const_cast<char*>(errno_descriptions[errnum]);
}

}