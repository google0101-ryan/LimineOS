global EnableAvx

EnableAvx:
    push rax
    push rcx
    push rdx

    xor rcx, rcx
    xgetbv
    or eax, 7
    xsetbv

    pop rdx
    pop rcx
    pop rax
    ret