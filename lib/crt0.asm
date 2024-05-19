global _start
extern main

_start:
    call main

    mov rax, 0
    syscall

_loop:
    pause
    jmp _loop