#include "pit.h"
#include <kernel/arch/x86/util/ports.h>
#include <kernel/arch/x86/idt.h>
#include <kernel/arch/x86/driver/apic.h>
#include <kernel/arch/x86/cpu.h>
#include <kernel/drivers/screen.h>
#include <kernel/arch/x86/util/spinlock.h>

static volatile uintmax_t ticks = 0;

void HandlePIT(IDT::stackframe_t*)
{
	if (ticks > 0) ticks--;

	lapic.EOI();
}

void PIT::Init()
{
	IDT::AddHandler(0x20, HandlePIT);

	int divisor = 1193180 / 100;
	outb(0x43, 0x36);
	outb(0x40, divisor & 0xff);
	outb(0x40, divisor >> 8);
}

void PIT::sleep(int ms)
{
	ticks = ms;

	while (ticks)
		asm volatile("pause");
}