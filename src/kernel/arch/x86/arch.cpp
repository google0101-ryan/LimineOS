#include "arch.h"
#include "gdt.h"
#include "idt.h"

#include <kernel/drivers/screen.h>
#include <kernel/limine.h>

#include <kernel/arch/x86/mem/pmm.h>
#include <kernel/arch/x86/mem/vmm.h>

#include <kernel/arch/x86/driver/apic.h>
#include <kernel/arch/x86/driver/pit.h>
#include <kernel/arch/x86/acpi/acpi.h>
#include <kernel/arch/x86/cpu.h>
#include <kernel/arch/x86/scheduler/scheduler.h>

static volatile limine_memmap_request mmap_req = 
{
	.id = LIMINE_MEMMAP_REQUEST,
	.response = nullptr
};

static volatile limine_smp_request smp_req =
{
	.id = LIMINE_SMP_REQUEST,
	.response = nullptr
};

volatile int num_started_cpus = 1;

void ApInit(limine_smp_info* smp)
{
	IDT::LoadIDT();

	GDT::LoadGDT(smp->processor_id);

	VirtualMemory::Init(smp->processor_id);

	lapic = LAPIC();

	lapic.InitTimer();

	printf("Started CPU core %d\n", smp->processor_id);

	num_started_cpus++;

	Arch::Halt();
}

void Arch::Halt()
{
	while (1) asm volatile("hlt");
}

extern void KThread();

void Arch::Init()
{
	GDT::Init();

	Terminal::SetScreenColors(0x00FF00, 0x000000);

	printf("[x]: Loaded GDT\n");

	IDT::Init();

	Terminal::SetScreenColors(0x00FF00, 0x000000);

	printf("[x]: Loaded IDT\n");

	if (!mmap_req.response)
	{
		printf("[ERROR]: Memory map corrupted or not provided!\n");
		Halt();
	}
	
	PMM::Init(mmap_req.response);

	Terminal::SetScreenColors(0x00FF00, 0x000000);

	printf("[x]: PMM initialized\n");

	VirtualMemory::Init(0);

	Terminal::SetScreenColors(0x00FF00, 0x000000);

	printf("[x]: VMM initialized\n");

	printf("[x]: Setting up LAPIC\n");

	lapic = LAPIC();

	Terminal::SetScreenColors(0x00FF00, 0x000000);

	printf("[x]: Setting up ACPI\n");

	ACPI::ParseACPI();

	Terminal::SetScreenColors(0x00FF00, 0x000000);

	printf("[x]: Setting up PIT\n");

	PIT::Init();

	Terminal::SetScreenColors(0x00FF00, 0x000000);
	
	printf("[x]: Initializing BSP LAPIC timer\n");

	lapic.InitTimer();

	Terminal::SetScreenColors(0x00FF00, 0x000000);

	// printf("[x]: Starting %d CPUs\n", ACPI::GetCPUCount()-1);

	// for (uint64_t i = 0; i < smp_req.response->cpu_count; i++)
	// {
	// 	if (smp_req.response->cpus[i]->lapic_id == smp_req.response->bsp_lapic_id) continue;

	// 	smp_req.response->cpus[i]->goto_address = ApInit;
	// }

	// while (num_started_cpus < smp_req.response->cpu_count) asm volatile("hlt");

	// printf("All CPU cores started\n");

	Scheduler::Init();

	Scheduler::AddThread((uint64_t)KThread, false);

	Arch::Halt();
}