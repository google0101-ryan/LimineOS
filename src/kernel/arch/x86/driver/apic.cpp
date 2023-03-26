#include "apic.h"

#include <kernel/arch/x86/cpu.h>
#include <kernel/arch/x86/mem/vmm.h>
#include <kernel/drivers/screen.h>
#include <kernel/arch/x86/driver/pit.h>
#include <kernel/arch/x86/idt.h>
#include <kernel/arch/x86/scheduler/scheduler.h>

#define rdmsr(msr, lo, hi) \
	__asm__ volatile("rdmsr" : "=a" (lo), "=d" (hi) : "c" (msr))
#define wrmsr(msr, lo, hi) \
	__asm__ volatile("wrmsr" : : "c" (msr), "a" (lo), "d" (hi))

#define MSR_IA32_APIC_BASE 0x01B
#define MSR_IA32_APIC_BASE_BASE (0xfffff<<12)

#define LAPIC_REG_SPI 0xF0

void LAPIC::WriteReg(uint32_t reg, uint32_t data)
{
	*(uint32_t*)((uint8_t*)base + reg) = data;
}

uint32_t LAPIC::ReadReg(uint32_t reg)
{
	return *(uint32_t*)((uint8_t*)base + reg);
}

LAPIC::LAPIC()
{
	uint32_t lo, hi;
	rdmsr(MSR_IA32_APIC_BASE, lo, hi);
	base = (uint8_t*)(uint64_t)(lo & MSR_IA32_APIC_BASE_BASE);
	lo |= 0x800;
	wrmsr(MSR_IA32_APIC_BASE, lo, hi);

	WriteReg(LAPIC_REG_SPI, 0x1FF);
}

uint64_t ticks_in_1ms = 0;

void HandleLAPICInterrupt(IDT::stackframe_t* regs)
{
	Scheduler::Schedule(regs);

	// We can use this to schedule tasks on a per-cpu basis
	lapic.EOI();
}

void LAPIC::InitTimer()
{
	IDT::AddHandler(0xFE, HandleLAPICInterrupt);

	WriteReg(0x3E0, 0x03);
	WriteReg(0x380, 0xFFFFFFFF);
	WriteReg(0x320, ReadReg(0x320) & ~(1 << 0x10));
	PIT::sleep(1);
	WriteReg(0x320, ReadReg(0x320) | (1 << 0x10));
	ticks_in_1ms = 0xFFFFFFFF - ReadReg(0x390);

	printf("%d ticks per ms\n", ticks_in_1ms);

	WriteReg(0x3E0, 0x03);
	WriteReg(0x320, (((ReadReg(0x320) & ~(0x03 << 17)) | (0x01 << 17)) & 0xFFFFFF00) | 0xFE);
	WriteReg(0x380, ticks_in_1ms);
	WriteReg(0x320, ReadReg(0x320) & ~(1 << 0x10));
}

void LAPIC::EOI()
{
	WriteReg(0xB0, 0);
}

void LAPIC::SendIPIToAll(uint32_t ipi_num)
{
	for (int i = 0; i < 256; i++)
	{
		if (!cpus[i].initialized) continue;

		uint32_t id = cpus[i].apic_id;

		WriteReg(0x310, id << 24);
		uint32_t cmd = ipi_num | (1 << 14);
		WriteReg(0x300, cmd);
	}
}

LAPIC lapic;