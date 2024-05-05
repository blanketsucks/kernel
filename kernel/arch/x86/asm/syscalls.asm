global _syscall_interrupt_handler
extern _syscall_handler

%include "kernel/arch/x86/asm/common.inc"

_syscall_interrupt_handler:
    pusha
    pushsg

    mov ax, KERNEL_DATA_SELECTOR
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    
    call _syscall_handler
    
    add esp, 4

    popsg
    popa

    iret