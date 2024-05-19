#include <include/util.h>
#include <include/cpu/idt.h>
#include <include/cpu/cpu.h>
#include <include/mm/liballoc.h>
#include <include/sched/scheduler.h>
#include <include/cpu/Syscall.h>

extern "C" void SyscallHandler(SavedRegs* regs)
{
    if (regs->rax == 0)
        Scheduler::DeleteCurThread();
    else if (regs->rax == 1)
        printf("%c", (int)regs->rbx);

    return;
}

extern "C" void syscall_handler();

#define MSR_STAR 0xC000'0081
#define MSR_LSTAR 0xC000'0082
#define MSR_CSTAR 0xC000'0083
#define MSR_SFMASK 0xC000'0084

void SysCalls::Init()
{
    uint64_t low = ((uint64_t)syscall_handler) & 0xFFFFFFFF;
    uint64_t high = ((uint64_t)syscall_handler) >> 32;
    asm volatile("wrmsr" ::"a"(low), "d"(high), "c"(MSR_LSTAR));

    // User CS selector set to this field + 16, SS this field + 8 
    uint32_t sysretSelectors = 0x10;
    // When syscall is called, CS set to field, SS this field + 8
    uint32_t syscallSelectors = 0x20;
    asm volatile("wrmsr" ::"a"(0), "d"((sysretSelectors << 16) | syscallSelectors), "c"(MSR_STAR));
    // SFMASK masks flag values
    // mask everything except reserved flags to disable IRQs when syscall is called
    asm volatile("wrmsr" ::"a"(0x3F7FD5U), "d"(0), "c"(MSR_SFMASK));

    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(0xC0000080));
    low |= 1; // SCE (syscall enable)
    asm volatile("wrmsr" :: "a"(low), "d"(high), "c"(0xC0000080));
}