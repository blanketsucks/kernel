%include "kernel/arch/common.inc"

KERNEL_CODE_SELECTOR equ 0x28
KERNEL_DATA_SELECTOR equ 0x30

USER_DATA_SELECTOR equ 0x38
USER_CODE_SELECTOR equ 0x40

%macro pushaq 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsp
    push rbp
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro popaq 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rbp
    add rsp, 8
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro