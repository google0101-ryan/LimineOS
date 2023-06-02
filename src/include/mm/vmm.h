#pragma once

#include <stdint.h>

enum PageFlags : uint64_t
{
    present = 1,
    readwrite = 2,
    user = 4,
    pagewritethrough = 8,
    cachedisable = 16,
    xd = (1ULL << 63)
};

constexpr uint64_t FLAG_MASK = 0x800000000000007F;

struct PageTable
{
    uint64_t pte[512];
};

struct PageDirectory
{
    uint64_t pde[512];
};

struct PageDirPtrTable
{
    uint64_t pdpte[512];
};

struct PML4Table
{
    uint64_t pdpte[512];
};

namespace VirtualMemory
{

extern PML4Table* kernelDir;

void Initialize();

void SwapToKernelPT();
void SwapToPageTable(PML4Table* page);

void MapPage(PML4Table* root, uint64_t vaddr, uint64_t paddr, uint64_t flags);

}