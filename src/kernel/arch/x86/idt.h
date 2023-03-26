#pragma once

#include <stdint.h>

namespace IDT
{

void Init();

void LoadIDT(); // Used by APs to avoid overwriting registered IRQ handlers

struct stackframe_t
{
	uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
	uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
	uint64_t int_num, err_code, rip, cs, rflags, rsp, ss;
};

typedef void (*inthandler_t)(stackframe_t*);

void AddHandler(int interrupt, inthandler_t handler);

}