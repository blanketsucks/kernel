global _start
extern _early_main

extern _kernel_start
extern _kernel_end

%include "kernel/arch/x86/asm/common.inc"

MULTIBOOT_ALIGN      equ 1 << 0
MULTIBOOT_MEMINFO    equ 1 << 1
MULTIBOOT_VIDEO_MODE equ 1 << 2

MULTIBOOT_FLAGS      equ MULTIBOOT_ALIGN | MULTIBOOT_MEMINFO | MULTIBOOT_VIDEO_MODE
MULTIBOOT_MAGIC      equ 0x1BADB002
MULTIBOOT_CHECKSUM   equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

; %1: Page table (virtual address)
; %2: Starting physical address
%macro map_page_table 2
    mov edi, %1 - KERNEL_VIRTUAL_BASE
    mov esi, %2
    call _map_page_table
%endmacro

; %1: Virtual address of the page table
; %2: Index of the page table entry
%macro map_kernel_page_directory 2
    mov ecx, %1 - KERNEL_VIRTUAL_BASE
    or ecx, 0x003
    mov dword [kernel_page_directory - KERNEL_VIRTUAL_BASE + %2 * 4], ecx
%endmacro

section .multiboot align=4
    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM

    times 5 dd 0

    dd 0   ; Graphics mode
    dd 640 ; Width
    dd 480 ; Height
    dd 32  ; Depth 

section .stack nobits align=16
stack:
    resb STACK_SIZE

section .bss nobits align=0x1000
boot_page_directory:
    resd KERNEL_PAGE_NUMBER
kernel_page_directory:
    resd (1024 - KERNEL_PAGE_NUMBER)
boot_page_table0:
    resd 1024
boot_page_table1:
    resd 1024

section .text

_start:
    map_page_table boot_page_table0, 0
    map_page_table boot_page_table1, (PAGE_SIZE * 1024)

    map_kernel_page_directory boot_page_table0, 0

    ; ecx contains the value of (boot_page_table0 | 0x3) from the previous macro call and we use it
    ; to identity map the first 4MB of memory
    mov dword [boot_page_directory - KERNEL_VIRTUAL_BASE], ecx 

    map_kernel_page_directory boot_page_table1, 1

    mov ecx, boot_page_directory - KERNEL_VIRTUAL_BASE
    mov cr3, ecx

    mov ecx, cr0
    or ecx, 0x80010000
    mov cr0, ecx

    lea ecx, [_entrypoint]
    jmp ecx

_entrypoint:
    mov dword [boot_page_directory], 0
    invlpg [0]

    mov esp, stack + STACK_SIZE

    ; Push multiboot header as first argument for main
    add ebx, KERNEL_VIRTUAL_BASE
    push ebx

    cli
    call _early_main

    cli
    
    hlt
    jmp $

_map_page_table:
    xor eax, eax

    .loop:
        mov edx, esi
        or edx, 0x003
        mov [edi + eax * 4], edx

        add esi, PAGE_SIZE
        inc eax

        cmp eax, 1024
        jl .loop ; We keep looping until we have mapped 1024 pages

    ret
