#pragma once

#include <stdint.h>
#include <stddef.h>

class IOAPIC
{
private:
	uint8_t* base;

	uint32_t read(size_t reg);
	void write(size_t reg, uint32_t data);
	void write64(size_t reg, uint64_t data);
public:
	IOAPIC(uint64_t baseAddr);

	void RedirectIRQ(uint8_t irq, uint8_t vec, uint32_t delivery);
};