#include <std/asserts.h>

#ifdef __KERNEL__
    #include <kernel/panic.h>
#else
    #include <stdlib.h>
#endif

namespace std {

[[noreturn]]
void assert_failed(StringView file, int line, StringView condition, StringView message) {
    dbgln("{}:{}: Assertion failed: {}", file, line, condition);

#ifdef __KERNEL__
    kernel::panic(message, file.data(), line);
#else
    dbgln("  {}", message);
    abort();
#endif

}

}
