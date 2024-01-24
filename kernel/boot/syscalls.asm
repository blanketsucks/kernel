extern _syscall_handler

global __handle_syscall
__handle_syscall:
    pusha
    push ds
    push es
    push fs
    push gs
    push esp
    
    call _syscall_handler
    
    add esp, 4
    pop gs
    pop fs
    pop es
    pop ds
    popa

    iret