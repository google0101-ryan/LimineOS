#pragma once

#include <stdint.h>
#include <kernel/limine.h>

namespace GDT
{

void Init();

void LoadGDT(int cpunum);

void SetKernelStack(uint64_t stack);

}