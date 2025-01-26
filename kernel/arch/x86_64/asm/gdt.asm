BITS 64

%include "kernel/arch/common.inc"

global _flush_gdt
global _flush_tss

__gdtr:
    .limit dw 0
    .base  dq 0

; void _flush_tss(u8 selector);
_flush_tss:
    mov ax, 0x28
    ltr ax

    ret

; void _flush_gdt(GDTPointer*);
_flush_gdt:
    mov rax, [rdi]
    lgdt [rax]

    push KERNEL_CODE_SELECTOR

    lea rax, [rel .reload_cs]
    push rax

    retfq
.reload_cs:
    mov ax, KERNEL_DATA_SELECTOR

    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ret