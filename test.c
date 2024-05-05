void exit() {
    char* foo = (char*)0;
    *foo = 120;
}

void _start() {
    asm volatile("movl $0xDEADCAFE, %eax");
    exit();
}