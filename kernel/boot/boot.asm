extern main

extern _kernel_start
extern _kernel_end

MULTIBOOT_ALIGN      equ 1 << 0
MULTIBOOT_MEMINFO    equ 1 << 1
MULTIBOOT_VIDEO_MODE equ 1 << 2

MULTIBOOT_FLAGS      equ MULTIBOOT_ALIGN | MULTIBOOT_MEMINFO | MULTIBOOT_VIDEO_MODE
MULTIBOOT_MAGIC      equ 0x1BADB002
MULTIBOOT_CHECKSUM   equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

KERNEL_VIRTUAL_BASE  equ 0xC0000000

section .multiboot align=4
    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM

    times 5 dd 0

    dd 0   ; Graphics mode
    dd 640 ; Width
    dd 480 ; Height
    dd 32  ; Depth 

section .data align=0x1000
header:
    dd 0

section .stack nobits align=16
stack:
    resb 16384 ; 16 KiB
stack.top:

section .bss nobits align=0x1000
boot_page_directory:
    resb 4096
boot_page_table:
    resb 4096

section .text

global _start
_start:
    mov [header - KERNEL_VIRTUAL_BASE], ebx

    mov edi, boot_page_table - KERNEL_VIRTUAL_BASE
    mov esi, 0
    mov ecx, 1023
.loop:
    cmp esi, _kernel_start
    jl .loop.inner

    cmp esi, _kernel_end - KERNEL_VIRTUAL_BASE
    jge .loop.end

    mov edx, esi
    or edx, 0x003
    mov [edi], edx
.loop.inner:
    add esi, 4096
    add edi, 4

    loop .loop
.loop.end:
    ; Map the VGA Address to 0xC03FF000
    mov eax, 0x000B8000 | 0x003
    mov dword [boot_page_table - KERNEL_VIRTUAL_BASE + 1023 * 4], eax

    mov eax, boot_page_table - KERNEL_VIRTUAL_BASE + 0x003

    mov dword [boot_page_directory - KERNEL_VIRTUAL_BASE + 0], eax
    mov dword [boot_page_directory - KERNEL_VIRTUAL_BASE + 768 * 4], eax

    mov ecx, boot_page_directory - KERNEL_VIRTUAL_BASE
    mov cr3, ecx

    mov ecx, cr0
    or ecx, 0x80010000
    mov cr0, ecx

    ; Jump to _entrypoint
    lea ecx, [_entrypoint]
    jmp ecx

_entrypoint:
    mov dword [boot_page_directory], 0
    invlpg [0]

    mov esp, stack.top

    ; Push multiboot header as first argument for main
    push dword [header]

    call main

    cli
    
    hlt
    jmp $