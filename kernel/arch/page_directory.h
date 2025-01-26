#pragma once

#include <kernel/common.h>

#ifdef __x86_64__
    #include <kernel/arch/x86_64/page_directory.h>
#elif __x86__
    #include <kernel/arch/x86/page_directory.h>
#else
    #error "Unsupported architecture"
#endif