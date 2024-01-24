#ifndef _LIBALLOC_H
#define _LIBALLOC_H

#include <stddef.h>

/** \defgroup ALLOCHOOKS liballoc hooks 
 *
 * These are the OS specific functions which need to 
 * be implemented on any platform that the library
 * is expected to work on.
 */

/** @{ */

struct liballoc_stats {
    unsigned long long allocated;
    unsigned long long inuse;
    long long warnings;
    long long errors;
    long long possible_overruns;
};

// If we are told to not define our own size_t, then we skip the define.
//#define _HAVE_UINTPTR_T
//typedef	unsigned long	uintptr_t;

//This lets you prefix malloc and friends
#define PREFIX(func)		k ## func

#ifdef __cplusplus
extern "C" {
#endif

extern void    *PREFIX(malloc)(size_t);				///< The standard function.
extern void    *PREFIX(realloc)(void*, size_t);		///< The standard function.
extern void    *PREFIX(calloc)(size_t, size_t);		///< The standard function.
extern void     PREFIX(free)(void*);					///< The standard function.

extern liballoc_stats liballoc_get_stats(void);


#ifdef __cplusplus
}
#endif


/** @} */

#endif

