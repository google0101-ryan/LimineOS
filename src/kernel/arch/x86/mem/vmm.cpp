#include "vmm.h"
#include "pmm.h"

#include <kernel/drivers/screen.h>
#include <kernel/limine.h>
#include <kernel/arch/x86/cpu.h>

// KERNEL MEMORY MAP
// 0x00000000 - 0x100000000 - Identity mapped
// 0xffff800000000000 - 0xffff800100000000 - Mapped based on hhdm
// 0xffffffff80000000 - 0xffffffff8xxxxxxx - Kernel, mapped based on the kernel file request

// Userspace mapping
// 0x00000000 - 0x100000000 - Identity mapped
// 0xffff800000000000 - 0xffff800100000000 - Mapped based on hhdm
// 0xffff900000000000 - 0xffff900000010000 - 64 KiB stack
// 0xffffa00000000000 - 0xffffffffaxxxxxxx - usermode code
// 0xffffffff80000000 - 0xffffffff8xxxxxxx - Kernel, mapped based on the kernel file request

extern "C" uint32_t _kernel_start;
extern "C" uint32_t _kernel_end;

static volatile limine_kernel_address_request addr_req = 
{
	.id = LIMINE_KERNEL_ADDRESS_REQUEST,
	.response = nullptr
};
static volatile limine_kernel_file_request file_req = 
{
	.id = LIMINE_KERNEL_FILE_REQUEST,
	.response = nullptr
};
static volatile limine_hhdm_request hhdm_req = 
{
	.id = LIMINE_HHDM_REQUEST,
	.response = nullptr
};

void VirtualMemory::MapPage(PageMapLevel4* pml4, uint64_t physaddr, uint64_t virtaddr, uint64_t flags)
{
	uint64_t xd = flags & (1ULL << 63);
	flags &= 0xfff;

	physaddr &= 0xfffffffff000;
	virtaddr &= ~0xfff;

	uint64_t pml4index = (virtaddr >> 39) & 0x1FF;
	uint64_t dirptrindex = (virtaddr >> 30) & 0x1FF;
	uint64_t dirindex = (virtaddr >> 21) & 0x1FF;
	uint64_t tableindex = (virtaddr >> 12) & 0x1FF;

	// We mark it RW, User, and present. PTEs will take care of specifics
	if (!pml4->entries[pml4index]) pml4->entries[pml4index] = ((uint64_t)PMM::AllocPage(1) & ~0xfff) | flags::present | flags::writeable | flags::user;

	PageDirectoryPointerTable* pdpt = (PageDirectoryPointerTable*)((uint64_t)pml4->entries[pml4index] & ~0xFFF);

	if (!pdpt->entries[dirptrindex]) pdpt->entries[dirptrindex] = ((uint64_t)PMM::AllocPage(1) & ~0xfff) | flags::present | flags::writeable | flags::user;

	PageDirectory* pd = (PageDirectory*)((uint64_t)pdpt->entries[dirptrindex] & ~0xFFF);

	if (!pd->entries[dirindex]) pd->entries[dirindex] = ((uint64_t)PMM::AllocPage(1) & ~0xfff) | flags::present | flags::writeable | flags::user;

	PageTable* pt = (PageTable*)((uint64_t)pd->entries[dirindex] & ~0xFFF);
	
	pt->entries[tableindex] = physaddr | flags | xd;
}

VirtualMemory::PageMapLevel4 *VirtualMemory::AllocateAddressSpace(uint64_t &stack, uint64_t stack_size)
{
	PageMapLevel4* pml4 = (PageMapLevel4*)PMM::AllocPage(1);

	printf("New pagemap at %p\n", pml4);

	for (uint64_t i = 0; i < 0x100000000; i += 0x1000)
	{
		MapPage(pml4, i, i, flags::writeable | flags::present);
		MapPage(pml4, i, hhdm_req.response->offset+i, flags::present | flags::writeable);
	}
	

	uintptr_t end = (uintptr_t)&_kernel_end;
	uintptr_t start = (uintptr_t)&_kernel_start;
	uintptr_t size = end - start;

	for (uint64_t i = 0; i < size; i += 0x1000)
		MapPage(pml4, addr_req.response->physical_base+i, addr_req.response->virtual_base+i, flags::writeable | flags::present);	

	uint64_t stack_addr = ((uint64_t)PMM::AllocPage(ALIGN(stack_size, 0x1000) / 0x1000));
	stack = 0xffff900000000000 + stack_size;

	for (uint64_t i = 0; i < ALIGN(stack_size, 0x1000); i += 0x1000)
		MapPage(pml4, stack_addr+i, 0xffff900000000000+i, flags::user | flags::writeable | flags::present);
	
	return pml4;
}

void VirtualMemory::SwitchPageMap(PageMapLevel4 *pml4)
{
	asm volatile("mov %0, %%cr3" :: "r"(pml4) : "memory");
}

void VirtualMemory::SwitchToPagemap(int cpunum)
{
	auto kernel_table = cpus[cpunum].top_lvl;
	SwitchPageMap(kernel_table);
}

void VirtualMemory::Init(int cpunum)
{
	cpus[cpunum].top_lvl = (PageMapLevel4*)PMM::AllocPage(1);
	auto kernel_table = cpus[cpunum].top_lvl;

	for (uint64_t i = 0; i < 0x100000000; i += 0x1000)
	{
		MapPage(kernel_table, i, i, flags::writeable | flags::present);
		MapPage(kernel_table, i, hhdm_req.response->offset+i, flags::present | flags::writeable);
	}

	uintptr_t end = (uintptr_t)&_kernel_end;
	uintptr_t start = (uintptr_t)&_kernel_start;
	uintptr_t size = end - start;

	for (uint64_t i = 0; i < size; i += 0x1000)
		MapPage(kernel_table, addr_req.response->physical_base+i, addr_req.response->virtual_base+i, flags::writeable | flags::present);	

	asm volatile("mov %0, %%cr3" :: "r"(kernel_table) : "memory");
}