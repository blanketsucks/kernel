[BITS 32]

global _start

global boot_pml4t
global boot_pdpt
global boot_pd
global kernel_pdpt
global kernel_pd

extern init

HIGHER_HALF_DIRECT_MAP equ 0xffff800000000000
HHDM_PML4E equ 0x800

VIDEO_ADDRESS equ 0xB8000
PAGE_SIZE equ 0x1000

%macro clear_page_tables 2
    mov edi, %1
    mov ecx, %2
    xor eax, eax
    rep stosd
%endmacro

MULTIBOOT_ALIGN      equ 1 << 0
MULTIBOOT_MEMINFO    equ 1 << 1
MULTIBOOT_VIDEO_MODE equ 1 << 2

MULTIBOOT_FLAGS      equ MULTIBOOT_ALIGN | MULTIBOOT_MEMINFO | MULTIBOOT_VIDEO_MODE
MULTIBOOT_MAGIC      equ 0x1BADB002
MULTIBOOT_CHECKSUM   equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

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
    resb 0x8000 ; 32KB
stack.bottom:

section .bss 
align PAGE_SIZE
boot_pml4t:
    resb PAGE_SIZE
boot_pdpt:
    resb PAGE_SIZE
boot_pd:
    resb PAGE_SIZE * 4
kernel_pdpt:
    resb PAGE_SIZE
kernel_pd:
    resb PAGE_SIZE * 2 ; The kernel will reside in the upper 2GB of memory

section .data
no_long_mode_support db "Long mode (64-bit) is not supported on this machine. Halting.", 0
no_pae_support db "PAE is not supported on this machine. Halting.", 0

gdt:
    .null: equ $ - gdt
        dq 0
    .code: equ $ - gdt
        dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)
    .ptr:
        dw $ - gdt - 1
        dq gdt

section .text

clear_screen:
    mov eax, 0x0020
    mov ecx, 80 * 25
    mov edi, VIDEO_ADDRESS
    rep stosw
    
    ret

; esi = String pointer (null-terminated).
;       If esi is null, halt without printing anything.
abort:
    test esi, esi
    jz .done

    mov ecx, VIDEO_ADDRESS
    mov ah, 0x07
.loop:
    lodsb
    test al, al
    jz .done

    mov word [ecx], ax
    add ecx, 2
    jmp .loop
.done:
    cli
    hlt

    jmp $

_start:
    push ebx

    call clear_screen

    ; Check if we can access CPUID with EAX=0x80000001
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .long_mode_not_supported

    ; Check if PAE is supported
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .pae_not_supported

    pop ebx
    
    clear_page_tables boot_pml4t, (PAGE_SIZE / 4) * 9 ; This will clear everything else below it too.

    ; Set boot_pml4t[0] to point to boot_pdpt
    mov edi, boot_pml4t

    mov dword [edi], boot_pdpt + 0x3
    mov dword [edi + HHDM_PML4E], boot_pdpt + 0x3
    
    ; Set boot_pdpt[0..3] to point to boot_pd[0..3]
    ; First check if 1GB pages are supported and use that instead.
    test edx, 1 << 26
    jz .no_1gb_pages

    mov edi, boot_pdpt

    mov dword [edi +  0], boot_pd + PAGE_SIZE * 0 + 0x83
    mov dword [edi +  8], boot_pd + PAGE_SIZE * 1 + 0x83
    mov dword [edi + 16], boot_pd + PAGE_SIZE * 2 + 0x83
    mov dword [edi + 24], boot_pd + PAGE_SIZE * 3 + 0x83

    jmp .loop.end
    
.no_1gb_pages:
    mov edi, boot_pdpt
    
    mov dword [edi +  0], boot_pd + PAGE_SIZE * 0 + 0x3
    mov dword [edi +  8], boot_pd + PAGE_SIZE * 1 + 0x3
    mov dword [edi + 16], boot_pd + PAGE_SIZE * 2 + 0x3
    mov dword [edi + 24], boot_pd + PAGE_SIZE * 3 + 0x3

    ; Identity map the first 4GB of memory
    mov edi, boot_pd
    mov ecx, 512 * 4
    mov eax, 0x83 ; Present and RW and PS (2MB page)

.loop:
    mov dword [edi], eax
    add eax, 0x200000 ; Add 2MB
    add edi, 8

    loop .loop

.loop.end:
    mov eax, boot_pml4t
    mov cr3, eax

    ; Enable PAE and PSE
    mov eax, cr4
    or eax, (1 << 5) | (1 << 4)
    mov cr4, eax

    ; Enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8)
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; Actually enter 64-bit mode by loading a 64-bit GDT
    lgdt [gdt.ptr]
    jmp gdt.code:_64bit_start

.long_mode_not_supported:
    mov esi, no_long_mode_support
    jmp abort

.pae_not_supported:
    mov esi, no_pae_support
    jmp abort

[BITS 64]

_64bit_start:
    cli

    mov rsp, stack.bottom + HIGHER_HALF_DIRECT_MAP
    and rsp, 0xFFFFFFFFFFFFFFF0 ; Align to 16 bytes

    mov ax, 0
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov rdi, rbx
    call init

    cli
    hlt

    jmp $