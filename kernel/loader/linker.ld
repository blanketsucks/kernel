ENTRY(_start)

SECTIONS {
    . = 0x100000;

    _loader_start = .;

    .text ALIGN(4K) : AT(ADDR(.text)) {
        *(.multiboot)
        *(.text*)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata)) {
        *(.rodata*)
    }

    .data ALIGN(4K) : AT(ADDR(.data)) {
        *(.data*)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss)) {
        *(COMMON)
        *(.bss)
        *(.stack)
    }

    _loader_end = .;
}