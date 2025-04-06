#include <kernel/memory/liballoc.h>
#include <kernel/memory/manager.h>

#include <kernel/common.h>

#include <stdint.h>

/**  Durand's Amazing Super Duper Memory functions.  */

#define VERSION 	"1.1"
#define ALIGNMENT	16ul//4ul				///< This is the byte alignment that memory must be allocated on. IMPORTANT for GTK and other stuff.

#define ALIGN_TYPE	char ///unsigned char[16] /// unsigned short
#define ALIGN_INFO	sizeof(ALIGN_TYPE)*16	///< Alignment information is stored right before the pointer. This is the number of bytes of information stored there.


#define USE_CASE1
#define USE_CASE2
#define USE_CASE3
#define USE_CASE4
#define USE_CASE5


/** This macro will conveniently align our pointer upwards */
#define ALIGN(ptr)													\
	if (ALIGNMENT > 1) {											\
		uintptr_t diff;												\
		ptr = (void*)((uintptr_t)ptr + ALIGN_INFO);					\
		diff = (uintptr_t)ptr & (ALIGNMENT-1);						\
		if (diff != 0) {											\
			diff = ALIGNMENT - diff;								\
			ptr = (void*)((uintptr_t)ptr + diff);					\
		}															\
		*((ALIGN_TYPE*)((uintptr_t)ptr - ALIGN_INFO)) = 			\
			diff + ALIGN_INFO;										\
	}


#define UNALIGN(ptr)													\
    if (ALIGNMENT > 1) {												\
        uintptr_t diff = *((ALIGN_TYPE*)((uintptr_t)ptr - ALIGN_INFO));	\
        if (diff < (ALIGNMENT + ALIGN_INFO)) {						    \
            ptr = (void*)((uintptr_t)ptr - diff);					    \
        }															    \
    }
				


#define LIBALLOC_MAGIC	0xc001c0de
#define LIBALLOC_DEAD	0xdeaddead

struct liballoc_minor;

/** A structure found at the top of all system allocated 
 * memory blocks. It details the usage of the memory block.
 */
struct liballoc_major {
	liballoc_major* prev; // Linked list information.
	liballoc_major* next; // Linked list information.

	u32 pages;			  // The number of pages in the block.
	u32 size;			  // The number of pages in the block.
	u32 usage;			  // The number of bytes used in the block.

	liballoc_minor* first; // A pointer to the first allocated memory in the block.	
};


/** This is a structure found at the beginning of all
 * sections in a major block which were allocated by a
 * malloc, calloc, realloc call.
 */
struct liballoc_minor {
	liballoc_minor* prev;  // Linked list information.
	liballoc_minor* next;  // Linked list information.
	liballoc_major* block; // The owning block. A pointer to the major structure.

	u32 magic;			   // A magic number to idenfity correctness.
	u32 size; 			   // The size of the memory allocated. Could be 1 byte or more.
	u32 req_size;		   // The size of memory requested.
};


static liballoc_major* l_memRoot = nullptr;	// The root memory block acquired from the system.
static liballoc_major* l_bestBet = nullptr;  // The major with the most free memory.

static u32 l_pageSize  = 4096;		// The size of an individual page. Set up in liballoc_init.
static u32 l_pageCount = 16;		// The number of pages to request per chunk. Set up in liballoc_init.
static u64 l_allocated = 0;		    // Running total of allocated memory.
static u64 l_inuse	 = 0;		    // Running total of used memory.

static i64 l_warningCount = 0;		// Number of warnings encountered
static i64 l_errorCount = 0;		// Number of actual errors
static i64 l_possibleOverruns = 0;	// Number of possible overruns


// ***********   HELPER FUNCTIONS  *******************************

static void* liballoc_memset(void* s, int c, size_t n) {
	for (unsigned int i = 0; i < n ; i++) {
		((char*)s)[i] = c;
    }
	
	return s;
}

static void* liballoc_memcpy(void* s1, const void* s2, size_t n) {
    char *cdest;
    char *csrc;
    unsigned int *ldest = (unsigned int*)s1;
    unsigned int *lsrc  = (unsigned int*)s2;

    while (n >= sizeof(unsigned int)) {
        *ldest++ = *lsrc++;
        n -= sizeof(unsigned int);
    }

    cdest = (char*)ldest;
    csrc  = (char*)lsrc;
    
    while (n > 0) {
        *cdest++ = *csrc++;
        n -= 1;
    }
    
    return s1;
}

// ***************************************************************

static struct liballoc_major* allocate_new_page(unsigned int size) {
    // This is how much space is required.
	unsigned int st = size + sizeof(struct liballoc_major) + sizeof(struct liballoc_minor);

    // Perfect amount of space?
    if ((st % l_pageSize) == 0) {
        st /= l_pageSize;
    } else {
        st = (st / l_pageSize) + 1; // No, add the buffer.
    }

    if (st < l_pageCount) st = l_pageCount;

    auto* major = (liballoc_major*)liballoc_alloc(st);
    if (!major) {
        l_warningCount += 1;
        return nullptr;
    }

    major->prev = nullptr;
    major->next = nullptr;
    major->pages = st;
    major->size = st * l_pageSize;
    major->usage = sizeof(struct liballoc_major);
    major->first = nullptr;

    l_allocated += major->size;
    return major;
}


void *PREFIX(malloc)(size_t req_size) {
	int startedBet = 0;
	void *p = NULL;
	uintptr_t diff;
	unsigned long size = req_size;

	// For alignment, we adjust size so there's enough space to align.
	if (ALIGNMENT > 1) {
		size += ALIGNMENT + ALIGN_INFO;
	}

    // So, ideally, we really want an alignment of 0 or 1 in order
    // to save space.
	
	liballoc_lock();
    if (size == 0) {
        l_warningCount += 1;

        liballoc_unlock();
        return PREFIX(malloc)(1);
    }

	if (!l_memRoot) {	
		// This is the first time we are being used.
		l_memRoot = allocate_new_page(size);
		if (!l_memRoot) {
            liballoc_unlock();
            return nullptr;
		}
	}

	// Now we need to bounce through every major and find enough space....

	liballoc_major* major = l_memRoot;
	startedBet = 0;

    u64 best_size = 0;
	
	// Start at the best bet....
	if (l_bestBet != NULL) {
		best_size = l_bestBet->size - l_bestBet->usage;

		if (best_size > (size + sizeof(liballoc_minor))) {
			major = l_bestBet;
			startedBet = 1;
		}
	}
	
	while (major) {
		diff = major->size - major->usage;// free memory in the block

		if (best_size < diff) {
			// Hmm.. this one has more memory then our bestBet. Remember!
			l_bestBet = major;
			best_size = diff;
		}


#ifdef USE_CASE1 // CASE 1:  There is not enough space in this major block.
    if (diff < (size + sizeof(liballoc_minor))) {	
        // Another major block next to this one?
        if (major->next) {
            major = major->next;		// Hop to that one.
            continue;
        }

        if (startedBet == 1) {		// If we started at the best bet,
            // let's start all over again.
            major = l_memRoot;
            startedBet = 0;

            continue;
        }

        major->next = allocate_new_page(size);
        if (!major->next) break;

        major->next->prev = major;
        major = major->next;

        // .. fall through to CASE 2 ..
    }

#endif

#ifdef USE_CASE2 // CASE 2: It's a brand new block.
    if (major->first == nullptr) {
        major->first = (liballoc_minor*)((uintptr_t)major + sizeof(liballoc_major));

        major->first->magic = LIBALLOC_MAGIC;
        major->first->prev = nullptr;
        major->first->next = nullptr;
        major->first->block = major;
        major->first->size = size;
        major->first->req_size = req_size;
        major->usage += size + sizeof(liballoc_minor);

        l_inuse += size;

        p = (void*)((uintptr_t)(major->first) + sizeof(liballoc_minor));
        ALIGN(p);

        liballoc_unlock();
        return p;
    }

#endif
				
#ifdef USE_CASE3 // CASE 3: Block in use and enough space at the start of the block.
    diff = (uintptr_t)(major->first);

    diff -= (uintptr_t)major;
    diff -= sizeof(liballoc_major);

    if (diff >= (size + sizeof(liballoc_minor))) {
        // Yes, space in front. Squeeze in.
        major->first->prev = (liballoc_minor*)((uintptr_t)major + sizeof(liballoc_major));
        major->first->prev->next = major->first;
        major->first = major->first->prev;

        major->first->magic = LIBALLOC_MAGIC;
        major->first->prev = nullptr;
        major->first->next = nullptr;
        major->first->block = major;
        major->first->size = size;
        major->first->req_size = req_size;
        major->usage += size + sizeof(liballoc_minor);

        l_inuse += size + sizeof(liballoc_minor);

        p = (void*)((uintptr_t)(major->first) + sizeof(liballoc_minor));
        ALIGN(p);

        liballoc_unlock();
        return p;
    }

#endif

#ifdef USE_CASE4 // CASE 4: There is enough space in this block. But is it contiguous?
    liballoc_minor* min = major->first;
    while (min != nullptr) {
        // CASE 4.1: End of minors in a block. Space from last and end?
        if (min->next == nullptr) {
            // the rest of this block is free...  is it big enough?
            diff = (uintptr_t)(major) + major->size;
            diff -= (uintptr_t)min;
            diff -= sizeof(liballoc_minor);
            diff -= min->size;

            if (diff >= (size + sizeof(liballoc_minor))) {
                // yay....
                min->next = (liballoc_minor*)((uintptr_t)min + sizeof(liballoc_minor) + min->size);
                min->next->prev = min;

                auto* next = min->next;
                next->next = nullptr;
                next->magic = LIBALLOC_MAGIC;
                next->block = major;
                next->size = size;
                next->req_size = req_size;
                major->usage += size + sizeof(liballoc_minor);

                l_inuse += size;

                p = (void*)((uintptr_t)next + sizeof(liballoc_minor));
                ALIGN(p);

                liballoc_unlock();
                return p;
            }
        }

        if (min->next != nullptr) {
            // CASE 4.2: Is there space between two minors?
            diff = (uintptr_t)(min->next);
            diff -= (uintptr_t)min;
            diff -= sizeof(liballoc_minor);
            diff -= min->size;

            if (diff >= (size + sizeof(liballoc_minor))) {
                // yay......
                auto* new_min = (liballoc_minor*)((uintptr_t)min + sizeof(liballoc_minor) + min->size);

                new_min->magic = LIBALLOC_MAGIC;
                new_min->next = min->next;
                new_min->prev = min;
                new_min->size = size;
                new_min->req_size = req_size;
                new_min->block = major;
                min->next->prev = new_min;
                min->next = new_min;
                major->usage += size + sizeof(liballoc_minor);
                
                l_inuse += size;
                
                p = (void*)((uintptr_t)new_min + sizeof(liballoc_minor));
                ALIGN(p);
                
                liballoc_unlock();		// release the lock
                return p;
            }
        }

        min = min->next;
    }

#endif

#ifdef USE_CASE5 // CASE 5: Block full! Ensure next block and loop.
    if (!major->next) {
        if (startedBet == 1) {
            major = l_memRoot;
            startedBet = 0;
            continue;
        }

        // we've run out. we need more...
        major->next = allocate_new_page(size);
        if (!major->next) break; // uh oh,  no more memory.....

        major->next->prev = major;
    }

#endif

		major = major->next;
	} // while (major)

	liballoc_unlock();		// release the lock
	return nullptr;
}


void PREFIX(free)(void *ptr) {
    if (!ptr) {
        l_warningCount += 1;
        return;
    }

	UNALIGN(ptr);

	auto* minor = (liballoc_minor*)((uintptr_t)ptr - sizeof(liballoc_minor));
    if (minor->magic != LIBALLOC_MAGIC) {
        l_errorCount += 1;
        return;
    }

	liballoc_lock();

	l_inuse -= minor->size;
	liballoc_major* major = minor->block;

	major->usage -= (minor->size + sizeof(liballoc_minor));
	minor->magic  = LIBALLOC_DEAD;	

	if (minor->next) minor->next->prev = minor->prev;
	if (minor->prev) minor->prev->next = minor->next;
	if (!minor->prev) major->first = minor->next;

	if (!major->first) {
		if (l_memRoot == major) l_memRoot = major->next;
		if (l_bestBet == major) l_bestBet = nullptr;
		if (major->prev) major->prev->next = major->next;
		if (major->next) major->next->prev = major->prev;

		l_allocated -= major->size;
		liballoc_free(major, major->pages);
	} else if (l_bestBet != nullptr) {
		int bestSize = l_bestBet->size - l_bestBet->usage;
		int majSize = major->size - major->usage;

		if (majSize > bestSize) l_bestBet = major;
	}
	
	liballoc_unlock();
}


void* PREFIX(calloc)(size_t nobj, size_t size) {
	size_t sz = nobj * size;
	void* p = PREFIX(malloc)(sz);

	liballoc_memset(p, 0, sz);
	return p;
}


void* PREFIX(realloc)(void* p, size_t size) {
	// Honour the case of size == 0 => free old and return NULL
	if (size == 0) {
		PREFIX(free)(p);
		return nullptr;
	}

	if (!p) {
		return PREFIX(malloc)(size);
	}

	// Unalign the pointer if required.
	void* ptr = p;
	UNALIGN(ptr);

	liballoc_lock();
	auto* minor = (liballoc_minor*)((uintptr_t)ptr - sizeof(liballoc_minor));
	if (minor->magic != LIBALLOC_MAGIC) {
		l_errorCount += 1;
		liballoc_unlock();

		return nullptr;
	}

	size_t sz = minor->req_size;
	if (sz >= size) {
		minor->req_size = size;
		liballoc_unlock();

		return p;
	}

	liballoc_unlock();

	// If we got here then we're reallocating to a block bigger than us.
	ptr = PREFIX(malloc)( size );	// We need to allocate new memory

	liballoc_memcpy( ptr, p, sz);
	PREFIX(free)(p);

	return ptr;
}
