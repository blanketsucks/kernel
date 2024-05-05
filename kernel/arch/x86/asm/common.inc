KERNEL_VIRTUAL_BASE  equ 0xC0000000
KERNEL_PAGE_NUMBER   equ KERNEL_VIRTUAL_BASE >> 22
PAGE_SIZE            equ 0x1000
STACK_SIZE           equ 0x4000

KERNEL_CODE_SELECTOR equ 0x08
KERNEL_DATA_SELECTOR equ 0x10

%macro pushsg 0
    push ds
    push es
    push fs
    push gs
%endmacro

%macro popsg 0
    pop gs
    pop fs
    pop es
    pop ds
%endmacro