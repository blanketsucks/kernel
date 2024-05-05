#pragma once

#ifdef __x86_64__
    #include <kernel/arch/x86_64/registers.h>
#elif __x86__
    #include <kernel/arch/x86/registers.h>
#else
    #error "Unsupported architecture"
#endif
