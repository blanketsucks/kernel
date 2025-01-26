%include "kernel/arch/common.inc"

KERNEL_VIRTUAL_BASE  equ 0xC0000000
KERNEL_PAGE_NUMBER   equ KERNEL_VIRTUAL_BASE >> 22
STACK_SIZE           equ 0x4000
