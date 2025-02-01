BITS 64

%include "kernel/arch/x86_64/common.inc"

global _isr_stub_table
global _irq_stub_table
global _default_interrupt_handler

extern _interrupt_exception_handler
extern _irq_handler

%macro define_common_stub 2
    _%1_common_stub:
        pushaq

        mov rdi, rsp
        call %2

        popaq

        add rsp, 16
        iretq
%endmacro

define_common_stub isr, _interrupt_exception_handler
define_common_stub irq, _irq_handler

_default_interrupt_handler:
    iret

%macro define_isr 1
    global _isr_stub_%1
    _isr_stub_%1:
        push 0
        push %1
        jmp _isr_common_stub
%endmacro

%macro define_isr_err 1
    global _isr_stub_%1
    _isr_stub_%1:
        push %1
        jmp _isr_common_stub
%endmacro

%macro define_irq 1
    global _irq_stub_%1
    _irq_stub_%1:
        push 0
        push %1 + 32
        jmp _irq_common_stub
%endmacro

define_isr 0
define_isr 1
define_isr 2
define_isr 3
define_isr 4
define_isr 5
define_isr 6
define_isr 7
define_isr_err 8
define_isr 9
define_isr_err 10
define_isr_err 11
define_isr_err 12
define_isr_err 13
define_isr_err 14
define_isr 15
define_isr 16
define_isr_err 17
define_isr 18
define_isr 19
define_isr 20
define_isr 21
define_isr 22
define_isr 23
define_isr 24
define_isr 25
define_isr 26
define_isr 27
define_isr 28
define_isr 29
define_isr_err 30
define_isr 31

%assign i 0
%rep 16
    define_irq i
    %assign i i+1
%endrep

_isr_stub_table:
%assign i 0 
%rep 32
    dq _isr_stub_%+i
    %assign i i+1 
%endrep

_irq_stub_table:
%assign i 0
%rep 16
    dq _irq_stub_%+i
    %assign i i+1
%endrep