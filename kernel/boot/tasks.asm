%include "kernel/boot/common.inc"

global _switch_context
global _switch_context_no_state
global _first_yield

struc Registers
    .gs resd 1
    .fs resd 1
    .es resd 1
    .ds resd 1
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

; void _switch_context_no_state(Registers*); 
;   Switch to a task without saving the current state. Used for the first task switch.
_switch_context_no_state:
    mov eax, [esp + 4]
    mov esp, [eax + Registers.esp0]

    pop ebp
    pop edi
    pop esi
    pop ebx

    ret
    
; void _switch_context(Registers* old, Registers* new);
_switch_context:
    push ebx
    push esi
    push edi
    push ebp

    mov edi, [esp + 20]
    mov [edi + Registers.esp0], esp ; Save current stack pointer to `old`

    ; Load esp and cr3 from `new`
    mov esi, [esp + 24]
    
    mov esp, [esi + Registers.esp0]
    mov eax, [esi + Registers.cr3]

    mov ecx, cr3

    cmp ecx, eax
    jz .no_cr3_change ; No need to cause a TLB flush if we are switching to the same page directory

    mov cr3, eax
.no_cr3_change:

    pop ebp
    pop edi
    pop esi
    pop ebx

    ret

_first_yield:
    popsg
    popad

    iret
