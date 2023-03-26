#pragma once

#include <stdint.h>

namespace VirtualMemory
{

namespace flags
{

enum : uint64_t
{
	present = 1,
	writeable = 2,
	user = 4,
	writethrough = 8,
	uncacheable = 16,
	pagesize = 128,
	executedisable = 0x8000000000000000
};

}

typedef uint64_t pml4entry;
typedef uint64_t pdptentry;
typedef uint64_t pdentry;

struct [[gnu::packed]] PageMapLevel4
{
	pml4entry entries[512];
};

struct [[gnu::packed]] PageDirectoryPointerTable
{
	pdptentry entries[512];
};

struct [[gnu::packed]] PageDirectory
{
	pdentry entries[512];
};

struct [[gnu::packed]] PageTable
{
	uint64_t entries[512];
};

static_assert(sizeof(PageMapLevel4) == 0x1000);
static_assert(sizeof(PageDirectoryPointerTable) == 0x1000);
static_assert(sizeof(PageDirectory) == 0x1000);
static_assert(sizeof(PageTable) == 0x1000);

void Init(int cpunum);

void MapPage(PageMapLevel4* pml4, uint64_t physaddr, uint64_t virtaddr, uint64_t flags);

// Create a basic userspace-thread page table, and return it
PageMapLevel4* AllocateAddressSpace(uint64_t& stack, uint64_t stack_size);

}