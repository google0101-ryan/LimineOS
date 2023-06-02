#pragma once

#include <stdint.h>

namespace LAPIC
{

void Initialize();

void WriteReg(uint8_t index, uint32_t val);
uint32_t ReadReg(uint8_t index);

}

namespace IOAPIC
{

void Initialize();

uint32_t ReadReg(uint32_t index);
void WriteReg(uint32_t index, uint32_t val);

}