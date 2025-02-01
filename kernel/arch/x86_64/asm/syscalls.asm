BITS 64

%include "kernel/arch/x86_64/common.inc"

extern _syscall_handler

global _syscall_interrupt_handler
_syscall_interrupt_handler:
    pushaq

    mov rdi, rsp
    call _syscall_handler

    popaq
    o64 sysret
