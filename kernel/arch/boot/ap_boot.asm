
section .ap_trampoline align=8

AP_BASE equ 0x8000

global ap_boot_start
global ap_boot_end

global ap_init_cr3
global ap_init_gdtr
global ap_init_idtr
global ap_init_stacks
global ap_entry_function

[BITS 16]
ap_boot_start:
    cli
    jmp 0x800:(.real - ap_boot_start)
.real:
    mov ax, cs
    mov ds, ax
    xor ax, ax
    mov ss, ax

    o32 lgdt [ap_boot_gdtr - ap_boot_start]

    mov eax, cr0
    or eax, 1 ; Enable Protected Mode
    mov cr0, eax

    jmp 0x08:(AP_BASE + (ap_boot_pm - ap_boot_start))

[BITS 32]
ap_boot_pm:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov eax, 1
    lock xadd [AP_BASE + (ap_cpu_id - ap_boot_start)], eax
    mov esi, eax

    ; Check for NX and enable it
    mov eax, 0x80000001
    cpuid
    test edx, (1 << 20)
    jz (AP_BASE + (.no_nx - ap_boot_start))

    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 11)
    wrmsr
.no_nx:
    mov eax, [AP_BASE + (ap_init_cr3 - ap_boot_start)]
    mov cr3, eax

    ; Enable PAE and PSE
    mov eax, cr4
    or eax, (1 << 5)
    mov cr4, eax

    ; Enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8)
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, (1 << 31)
    mov cr0, eax

    jmp 0x18:(AP_BASE + (ap_boot_lm - ap_boot_start))

[BITS 64]
ap_boot_lm:
    mov rsp, [AP_BASE + (ap_init_stacks - ap_boot_start) + rsi * 8]

    mov rdi, rsi
    inc rdi

    mov r10, [AP_BASE + (ap_entry_function - ap_boot_start)]
    call r10

    hlt
    jmp $


align 8

ap_init_cr3:
    dq 0
ap_entry_function:
    dq 0

ap_cpu_id:
    dq 0
ap_boot_gdt:
    dq 0
    dq 0x00CF9A000000FFFF ; 0x8 : 32-bit code
    dq 0x00CF92000000FFFF ; 0x10: 32-bit data
    dq 0x00AF9A000000FFFF ; 0x18: 64-bit code
ap_boot_gdtr:
    dw ap_boot_gdtr - ap_boot_gdt - 1
    dd AP_BASE + (ap_boot_gdt - ap_boot_start)

ap_init_stacks:

ap_boot_end: