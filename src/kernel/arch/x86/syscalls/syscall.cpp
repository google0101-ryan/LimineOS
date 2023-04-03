#include "syscall.h"

#include <stdint.h>
#include <kernel/arch/x86/idt.h>
#include <kernel/drivers/screen.h>
#include <kernel/arch/x86/arch.h>
#include <kernel/arch/x86/cpu.h>

extern "C" void SyscallStub();

#define MSR_STAR 0xC000'0081
#define MSR_LSTAR 0xC000'0082
#define MSR_CSTAR 0xC000'0083
#define MSR_SFMASK 0xC000'0084

void Syscall::Init()
{
    uint64_t low = ((uint64_t)SyscallStub) & 0xFFFFFFFF;
    uint64_t high = ((uint64_t)SyscallStub) >> 32;
    asm volatile("wrmsr" :: "a"(low), "d"(high), "c"(MSR_LSTAR));

    uint32_t sysretSelectors = 0x23 - 8;
    uint32_t syscallSelectors = 0x08;
    asm volatile("wrmsr" :: "a"(0), "d"((sysretSelectors << 16) | syscallSelectors), "c"(MSR_STAR));

    asm volatile("wrmsr" :: "a"(0x3F7FD5U), "d"(0), "c"(MSR_SFMASK));

    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(0xC0000080));
    low |= 1;
    asm volatile("wrmsr" :: "a"(low), "d"(high), "c"(0xC0000080));

    SetLocal(&cpus[0]);
}

extern "C" void SyscallHandler(IDT::stackframe_t* regs)
{
    switch (regs->rax)
    {
    case 1:
        Terminal::Log((const char*)regs->rdi);
        regs->rax = 0;
        break;
    default:
        printf("Unhandled syscall 0x%x\n", regs->rax);
        Arch::Halt();
    }
}