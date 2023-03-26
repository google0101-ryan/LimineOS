#pragma once

#include <stdint.h>
#include <kernel/arch/x86/mem/vmm.h>

class LAPIC;

struct CPU
{
	uint64_t apic_id;
	VirtualMemory::PageMapLevel4* top_lvl;
	bool initialized = false;
};

extern CPU cpus[256];

extern LAPIC lapic;