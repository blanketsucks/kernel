BITS 64

%include "kernel/arch/x86_64/common.inc"

global _flush_gdt

; void _flush_gdt(GDTPointer*);
_flush_gdt:
    lgdt [rdi]

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