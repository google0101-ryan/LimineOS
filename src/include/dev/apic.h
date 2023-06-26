#pragma once

#include <stdint.h>

namespace LAPIC
{

void Initialize();
void InitTimer(bool is_bsp = false); // Initialize LAPIC timer to interrupt us at a fixed rate

void WriteReg(uint32_t index, uint32_t val);
uint32_t ReadReg(uint32_t index);

void EOI();

}

namespace IOAPIC
{

void Initialize();

uint32_t ReadReg(uint32_t index);
void WriteReg(uint32_t index, uint32_t val);

}