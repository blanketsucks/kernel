#include <kernel/common.h>
#include <kernel/efi/uefi.h>

#include <std/elf.h>

asm(
    ".section .text\n"
    ".align 4\n"
    ".globl _start\n"
    "_start:\n"
    "   lea ImageBase(%rip), %rdi\n"
    "   lea _DYNAMIC(%rip), %rsi\n"
    "   call _relocate\n"
    "   ret\n"

    ".section .data\n"
    "dummy: .long 0\n"

    // Create a relocation entry so that EFI does not complain.
    ".section .reloc, \"a\"\n"
    "dummy1:\n"
    ".long dummy - dummy1\n"
    ".long 10\n"
    ".word 0\n"

    ".section .text\n"
);

extern EFIStatus efi_main(EFIHandle, EFISystemTable*);

extern "C" EFIStatus _relocate(uintptr_t ldbase, Elf64_Dyn* dyn, EFISystemTable* sys_table, EFIHandle image_handle) {
    u64 relsz = 0;
    u64 relent;

    Elf64_Rel* rel = nullptr;
    uintptr_t* addr = nullptr;

    while (dyn->d_tag != DT_NULL) {
        switch (dyn->d_tag) {
            case DT_RELA: {
                rel = (Elf64_Rel*)(ldbase + dyn->d_un.d_ptr);
                break;
            }
            case DT_RELASZ: {
                relsz = dyn->d_un.d_val;
                break;
            }
            case DT_RELAENT: {
                relent = dyn->d_un.d_val;
                break;
            }
            default: break;
        }

        ++dyn;
    }

    if (rel && relent) {
        while (relsz > 0) {
            if (ELF64_R_TYPE(rel->r_info) == R_X86_64_RELATIVE) {
                addr = (uintptr_t*)(ldbase + rel->r_offset);
                *addr += ldbase;
            }

            rel = (Elf64_Rel*)((u8*)rel + relent);
            relsz -= relent;
        }
    }
    
    return efi_main(image_handle, sys_table);
}