#pragma once

#include <stdint.h>

struct SavedRegs
{
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no;
    uint64_t error_code;
    uint64_t rip, cs, rflags, rsp, ss;
};

typedef SavedRegs* (*idt_handler_t)(SavedRegs*);

namespace IDT
{

struct [[gnu::packed]] IDTEntry
{
    uint16_t offset0;
    uint16_t cs;
    uint8_t ist;
    uint8_t flags; // Gate type, DPL, present bit
    uint16_t offset1;
    uint32_t offset2;
    uint32_t zero;
};

struct [[gnu::packed]] IDTR
{
    uint16_t size;
    uint64_t offset;
};

void Initialize();

void AddHandler(int no, idt_handler_t handler);

}