#pragma once

#include <stdint.h>
#include <kernel/arch/x86/scheduler/thread.h>
#include <kernel/arch/x86/idt.h>

namespace Scheduler
{

void Init();

Thread* AddThread(uint64_t entrypoint, bool user);

void Schedule(IDT::stackframe_t* regs);

}