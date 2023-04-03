%macro pushall 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro popall 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

extern SyscallHandler
global SyscallStub
SyscallStub:
    swapgs
    mov qword[gs:8], rsp
    mov rsp, qword[gs:0]
    push 0x23
    push qword[gs:8]
    swapgs
    push r11
    push 0x1B
    push rcx

    push 0
    push 0

    pushall

    mov rdi, rsp
    xor rbp, rbp
    call SyscallHandler

    popall

    add rsp, 16

    pop rcx
    add rsp, 8
    pop r11
    pop rsp
    o64 sysret
