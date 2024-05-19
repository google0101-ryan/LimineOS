#include <drivers/nvme.h>

#include <mm/vmm.h>
#include <mm/pmm.h>
#include <drivers/hpet.h>
#include <screen.h>

#define NVME_VIRT_ADDR 0xffffc00000480000

struct [[gnu::packed]] NvmeRegisters
{
	uint64_t cap; // Capabilities
	uint32_t version; // Version
	uint32_t intMask; // Interrupt Mask Set
	uint32_t intMaskClear; // Interrupt Mask Clear
	uint32_t config; // Controller Configuration
	uint32_t reserved;
	uint32_t status; // Controller status
	uint32_t nvmSubsystemReset; // NVM subsystem reset
	uint32_t adminQAttr; // Admin queue attributes
	uint64_t adminSubmissionQ; // Admin submission queue
	uint64_t adminCompletionQ; // Admin completion queue
	uint32_t controllerMemBufferLocation; // Controller memory buffer location (Optional)
	uint32_t controllerMemBufferSize; // Controller memory buffer size (Optional)
	uint32_t bootPartitionInfo; // Boot partition information (Optional)
	uint32_t bootPartitionReadSelect; // Boot partition read select (Optional)
	uint64_t bootPartitionMemBufferLocation; // Boot partition memory buffer location (Optional)
	uint64_t bootPartitionMemSpaceControl; // Boot partition memory space control (Optional)
	uint64_t controllerMemBufferMemSpaceControl; // Controller memory buffer memory space control (Optional)
	uint32_t controllerMemoryBufferStatus; // Controller memory buffer status (Optional)
} *nvmeRegs;

void* acqBase, *asqBase;

extern void WritePciRegister16(int bus, int device, int func, uint8_t offs, uint16_t data);
extern uint16_t ReadPciRegister16(int bus, int device, int func, uint8_t offs);

struct [[gnu::packed]] NVMeCommand
{
	struct
	{
		uint32_t opcode : 8;
		uint32_t fuse : 2;
		uint32_t : 4;
		uint32_t prp : 2;
		uint32_t cid : 16;
	};
	
};

bool NVMe::Init(PciDeviceInfo& info)
{
	WritePciRegister16(0, 3, 0, 0x4, ReadPciRegister16(0, 3, 0, 0x4) | (1 << 2) | (1 << 1));

	uint64_t acqPhysBase = (uint64_t)PhysicalMemory::alloc(1);
	uint64_t asqPhsyBase = (uint64_t)PhysicalMemory::alloc(1);

	for (int i = 0; i < 4; i++)
		VirtualMemory::MapPage(VirtualMemory::GetCurrentPML4(), NVME_VIRT_ADDR+i*4096, info.bar0+i*4096, PageFlags::present | PageFlags::readwrite | PageFlags::cachedisable);
	VirtualMemory::MapPage(VirtualMemory::GetCurrentPML4(), NVME_VIRT_ADDR+4*4096, acqPhysBase, PageFlags::present | PageFlags::readwrite | PageFlags::cachedisable);
	VirtualMemory::MapPage(VirtualMemory::GetCurrentPML4(), NVME_VIRT_ADDR+5*4096, asqPhsyBase, PageFlags::present | PageFlags::readwrite | PageFlags::cachedisable);
	
	acqBase = (void*)(NVME_VIRT_ADDR+4*4096);
	asqBase = (void*)(NVME_VIRT_ADDR+5*4096);

	nvmeRegs = (NvmeRegisters*)NVME_VIRT_ADDR;
	nvmeRegs->config &= ~1;
	
	while ((nvmeRegs->status & 1))
		asm volatile("pause");
	
	nvmeRegs->config &= ~(0xf << 7);
	nvmeRegs->config &= ~(0x7 << 4);

	nvmeRegs->config |= (4 << 20) | (6 << 16);

	nvmeRegs->adminCompletionQ = acqPhysBase;
	nvmeRegs->adminSubmissionQ = asqPhsyBase;

	nvmeRegs->adminQAttr = 0;
	nvmeRegs->adminQAttr |= (4095 << 16) | 4095;

	nvmeRegs->config |= 1;

	while (!(nvmeRegs->status & 1))
		asm volatile("pause");

	printf("[NVMe]: Drive reset, initializing\n");

	return true;
}