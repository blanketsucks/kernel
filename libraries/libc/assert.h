#include <sys/cdefs.h>

__BEGIN_DECLS

#ifdef NDEBUG
	#define assert(ignore) ((void)0)
#else
	[[gnu::noreturn]] void __assert_failed(const char* file, int line, const char* func, const char* expr);

	#define assert(expr) ((expr) ? (void) 0 : __assert_failed(__FILE__, __LINE__, __FUNCTION__, #expr))
#endif //NDEBUG

__END_DECLS