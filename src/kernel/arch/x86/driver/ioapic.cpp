#include "ioapic.h"

#define IO_APIC_RED_TABLE_ENT(x) (0x10 + 2 * x)

uint32_t IOAPIC::read(size_t reg)
{
	*(uint32_t*)&base[0] = reg & 0xFF;
	return *(uint32_t*)&base[16];
}

void IOAPIC::write(size_t reg, uint32_t data)
{
	*(uint32_t*)&base[0] = reg & 0xFF;
	*(uint32_t*)&base[16] = data;
}

void IOAPIC::write64(size_t reg, uint64_t data)
{
	uint32_t low = data & 0xFFFFFFFF;
    uint32_t high = (data >> 32) & 0xFFFFFFFF;

    write(reg, low);
    write(reg+1, high);
}

IOAPIC::IOAPIC(uint64_t baseAddr)
{
	base = (uint8_t*)baseAddr;
}

void IOAPIC::RedirectIRQ(uint8_t irq, uint8_t vec, uint32_t delivery)
{
	write64(IO_APIC_RED_TABLE_ENT(irq), vec | delivery);
}
