%include "kernel/arch/x86/common.inc"

global _flush_gdt
global _flush_tss

; void _flush_tss(u8 selector);
_flush_tss:
    mov ax, 0x28
    ltr ax

    ret

; void _flush_gdt(GDTPointer*);
_flush_gdt:
    mov eax, [esp + 4]
    lgdt [eax]

    mov ax, KERNEL_DATA_SELECTOR
    
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp KERNEL_CODE_SELECTOR:.flush_gdt_done
.flush_gdt_done:
    ret