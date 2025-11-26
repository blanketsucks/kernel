OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

SECTIONS {
    . = SEGMENT_START("text", 0x1000) + SIZEOF_HEADERS;

    .text : {
        *(.text.entry)
        *(.text .text.*)
    }

    .rodata : {
        *(.rodata .rodata.*)
    }

    .data : {
        *(.data .data.*)
        *(.bss .bss.*)
        *(COMMON)
    }
}
