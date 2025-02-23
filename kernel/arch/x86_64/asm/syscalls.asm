BITS 64

%include "kernel/arch/x86_64/common.inc"

USER_STACK_OFFSET   equ 0x00
KERNEL_STACK_OFFSET equ 0x0C

extern _syscall_handler

global _syscall_interrupt_handler
_syscall_interrupt_handler:
    ; Save the user stack and swap it with the kernel stack
    mov gs:USER_STACK_OFFSET, rsp
    mov rsp, gs:KERNEL_STACK_OFFSET

    push USER_DATA_SELECTOR | 3   ; ss
    push qword gs:USER_STACK_OFFSET
    push r11                      ; r11 contains the rflags after the `syscall` instruction
    push USER_CODE_SELECTOR | 3   ; cs
    push rcx                      ; rcx contains the return address

    sti
    pushaq

    mov rdi, rsp
    call _syscall_handler

    popaq

    pop rcx
    add rsp, 8
    pop r11
    pop rsp

    cli ; The sysret will enable interrupts again
    o64 sysret
