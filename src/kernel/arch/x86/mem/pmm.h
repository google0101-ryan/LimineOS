#pragma once

#include <stdint.h>
#include <stddef.h>
#include <kernel/limine.h>

#define __ALIGN_MASK(x, mask)    (((x) + (mask)) & ~(mask))
#define ALIGN(x, a)            __ALIGN_MASK(x,(__typeof(x))(a) - 1)

namespace PMM
{

void Init(limine_memmap_response* mmap);

void* AllocPage(int count);
void FreePage(void* page, size_t count);

}