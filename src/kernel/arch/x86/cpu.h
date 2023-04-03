#pragma once

#include <stdint.h>
#include <kernel/arch/x86/mem/vmm.h>

class LAPIC;

struct CPU
{
	uint64_t stack;
	uint64_t savedStack;
	uint64_t apic_id;
	VirtualMemory::PageMapLevel4* top_lvl;
	bool initialized = false;
};

extern CPU cpus[256];

extern LAPIC lapic;

static inline void SetLocal(CPU* val)
{
	asm volatile("wrmsr" ::"a"((uintptr_t)val & 0xFFFFFFFF) /*Value low*/,
                 "d"(((uintptr_t)val >> 32) & 0xFFFFFFFF) /*Value high*/, "c"(0xC0000102) /*Set Kernel GS Base*/);
    asm volatile("wrmsr" ::"a"((uintptr_t)val & 0xFFFFFFFF) /*Value low*/,
                 "d"(((uintptr_t)val >> 32) & 0xFFFFFFFF) /*Value high*/, "c"(0xC0000101) /*Set Kernel GS Base*/);
}