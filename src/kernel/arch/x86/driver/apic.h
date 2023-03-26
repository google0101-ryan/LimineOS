#pragma once

#include <stdint.h>

class LAPIC
{
private:
	volatile uint8_t* base;

	void WriteReg(uint32_t reg, uint32_t data);
	uint32_t ReadReg(uint32_t reg);
public:
	LAPIC();

	void InitTimer();

	void EOI();

	void SendIPIToAll(uint32_t ipi_num);
};