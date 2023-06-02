global isr_table

%macro push_all 0

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

%macro pop_all 0
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

extern interrupt_handler
isr_common:
    push_all

    mov rdi, rsp

    call interrupt_handler
    mov rsp, rax
    pop_all

    add rsp, 16
    sti
    iretq

%macro isr_noerr 1
isr_%1:
    push qword 0
    push qword %1
    cld
    jmp isr_common
%endmacro

%macro isr_err 1
isr_%1:
    push qword %1
    cld
    jmp isr_common
%endmacro

%macro INTERRUPT_NAME 1
dq isr_%1
%endmacro

%assign i 0
%rep 256
%if (i == 8 || (i >= 10 && i <= 14) || i == 17 || i == 21 || i == 29 || i == 30)
isr_err i
%else
isr_noerr i
%endif
%assign i i+1
%endrep

isr_table:
%assign i 0
%rep 256
INTERRUPT_NAME i
%assign i i+1
%endrep

global LoadIdt
LoadIdt:
    lidt [rdi]
    ret