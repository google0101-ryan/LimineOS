global TestAndSet
TestAndSet:
	mov rax, rdi
	mov rdx, rsi
	lock xchg [rdx], rax
	ret