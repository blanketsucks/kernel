ENTRY(_start)

KERNEL_VIRTUAL_BASE = 0xC0000000;

SECTIONS {
    . = 0xC0100000;

    _kernel_start = .;

    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VIRTUAL_BASE) {
        *(.multiboot)
        *(.text)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VIRTUAL_BASE) {
        _start_ctors = .;
        *(.init_array) *(.ctors) /* .init_array is used by clang and .ctors by gcc */
        _end_ctors = .;

        _start_dtors = .;
        *(.fini_array) *(.dtors) /* .fini_array is used by clang and .dtors by gcc */
        _end_dtors = .;

        *(.rodata)
    }

    .data ALIGN(4K) : AT (ADDR(.data) - KERNEL_VIRTUAL_BASE) {
        *(.data)
    }

    .bss ALIGN(4K) : AT (ADDR(.bss) - KERNEL_VIRTUAL_BASE) {
        *(COMMON)
        *(.bss)
        *(.stack)
    }

    _kernel_end = .;
}