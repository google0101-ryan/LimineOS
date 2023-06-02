global LoadGdt
LoadGdt:
    lgdt [rdi]
    mov ax, 0x10

    mov ds, ax
    mov es, ax
    mov ss, ax

    mov rax, qword .trampoline
    push qword 0x8
    push rax

    o64 retf

.trampoline:
    ret