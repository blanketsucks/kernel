#include <kernel/arch/arch.h>
#include <kernel/arch/x86/idt.h>
#include <kernel/arch/x86/gdt.h>

namespace kernel::arch {

void init() {
    arch::init_gdt();
    arch::init_idt();
}

}