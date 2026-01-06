BITS 64

%include "kernel/arch/x86_64/common.inc"

global _switch_context
global _switch_context_no_state
global _thread_first_enter

extern _thread_context_init

struc ThreadRegisters
    .r15 resq 1
    .r14 resq 1
    .r13 resq 1
    .r12 resq 1
    .r11 resq 1
    .r10 resq 1
    .r9 resq 1
    .r8 resq 1
    .rsi resq 1
    .rdi resq 1
    .rbp resq 1
    .rsp0 resq 1
    .rdx resq 1
    .rcx resq 1
    .rbx resq 1
    .rax resq 1
    .rip resq 1
    .cs resq 1
    .rflags resq 1
    .rsp resq 1
    .ss resq 1
    .cr3 resq 1
endstruc

%macro pushr 0
    pushfq
    push r15
    push r14
    push r13
    push r12
    push rbp
    push rbx
%endmacro

%macro popr 0
    pop rbx
    pop rbp
    pop r12
    pop r13
    pop r14
    pop r15
    popfq
%endmacro

_switch_context_no_state:
    mov rsp, [rdi + ThreadRegisters.rsp]

    popr
    ret

; void _switch_context(ThreadRegisters *old, ThreadRegisters *new)
_switch_context:
    pushr

    mov [rdi + ThreadRegisters.rsp], rsp

    ; Load rsp and cr3 from `new`
    mov rsp, [rsi + ThreadRegisters.rsp]
    mov rax, [rsi + ThreadRegisters.cr3]

    mov cr3, rax

    popr
    ret

_thread_first_enter:
    pop rdi
    call _thread_context_init

    popaq
    iretq