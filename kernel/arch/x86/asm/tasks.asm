%include "kernel/arch/x86/common.inc"

global _switch_context
global _switch_context_no_state
global _first_yield

struc ThreadRegisters
    .edi resd 1
    .esi resd 1
    .ebp resd 1
    .esp0 resd 1
    .ebx resd 1
    .edx resd 1
    .ecx resd 1
    .eax resd 1
    .eip resd 1
    .cs resd 1
    .eflags resd 1
    .esp resd 1
    .ss resd 1
    .cr3 resd 1
endstruc

; void _switch_context_no_state(ThreadRegisters*); 
;   Switch to a task without saving the current state. Used for the first task switch.
_switch_context_no_state:
    mov eax, [esp + 4]
    mov esp, [eax + ThreadRegisters.esp0]

    pop ebp
    pop edi
    pop esi
    pop ebx
    popfd

    ret
    
; void _switch_context(ThreadRegisters* old, ThreadRegisters* new);
_switch_context:
    ; Push callee-saved registers
    pushfd
    push ebx
    push esi
    push edi
    push ebp

    mov edi, [esp + 24]
    mov [edi + ThreadRegisters.esp0], esp ; Save current stack pointer to `old`

    ; Load esp and cr3 from `new`
    mov esi, [esp + 28]
    
    mov esp, [esi + ThreadRegisters.esp0]
    mov eax, [esi + ThreadRegisters.cr3]

    mov ecx, cr3

    cmp ecx, eax
    jz .no_cr3_change ; No need to cause a TLB flush if we are switching to the same page directory

    mov cr3, eax
.no_cr3_change:

    pop ebp
    pop edi
    pop esi
    pop ebx
    popfd

    ret

_first_yield:
    popsg
    popad

    iret
