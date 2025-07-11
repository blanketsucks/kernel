#pragma once

#ifdef __x86_64__
    #include <kernel/arch/x86_64/tss.h>
#elif __x86__
    #include <kernel/arch/x86/tss.h>
#else
    #error "Unsupported architecture"
#endif