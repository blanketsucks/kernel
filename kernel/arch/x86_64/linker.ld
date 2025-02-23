OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_early_main)

PHDRS {
    text    PT_LOAD    FLAGS((1 << 0) | (1 << 2)); /* Execute + Read */
    rodata  PT_LOAD    FLAGS((1 << 2));            /* Read only */
    data    PT_LOAD    FLAGS((1 << 1) | (1 << 2)); /* Write + Read */
    dynamic PT_DYNAMIC FLAGS((1 << 1) | (1 << 2)); /* Dynamic PHDR for relocations */
}

SECTIONS {
    . = 0xffffffff80000000;

    _kernel_start = .;
 
    .text : {
        *(.text .text.*)
    } : text

    . += CONSTANT(MAXPAGESIZE);

    .rodata : {
        *(.rodata .rodata.*)

        _start_ctors = .;
        *(.init_array) *(.ctors) /* .init_array is used by clang and .ctors by gcc */
        _end_ctors = .;

        _start_dtors = .;
        *(.fini_array) *(.dtors) /* .fini_array is used by clang and .dtors by gcc */
        _end_dtors = .;
    } : rodata

    . += CONSTANT(MAXPAGESIZE);

    .data : {
        *(.data .data.*)
    } : data

    .dynamic : {
        *(.dynamic)
    } : data : dynamic

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } : data

    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }
    
    _kernel_end = .;
}