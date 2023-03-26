#include "acpi.h"
#include <kernel/limine.h>
#include <kernel/drivers/screen.h>
#include <kernel/libk/string.h>
#include <kernel/arch/x86/arch.h>
#include <kernel/arch/x86/driver/ioapic.h>
#include <kernel/arch/x86/cpu.h>

static volatile limine_rsdp_request rsdp_req = 
{
	.id = LIMINE_RSDP_REQUEST,
	.response = nullptr
};

struct RSDPointer
{
	char Signature[8];
	uint8_t Checksum;
	char OEMID[6];
	uint8_t Revision;
	uint32_t RsdtAddress;
};

struct [[gnu::packed]] ACPISDTHeader 
{
	char Signature[4];
	uint32_t Length;
	uint8_t Revision;
	uint8_t Checksum;
	char OEMID[6];
	char OEMTableID[8];
	uint32_t OEMRevision;
	uint32_t CreatorID;
	uint32_t CreatorRevision;
} *rsdt;

struct [[gnu::packed]] MADTHeader
{
    ACPISDTHeader sdt;
    uint32_t local_controller_addr;
    uint32_t flags;
    char entries_begin[];
};

void* FindTable(uint32_t signature, size_t skip)
{
	if (skip < 0) skip = 0;
	size_t entries = (rsdt->Length - sizeof(ACPISDTHeader)) / 4;

	for (size_t i = 0; i < entries; i++)
	{
		ACPISDTHeader* hdr = reinterpret_cast<ACPISDTHeader*>(*reinterpret_cast<uint32_t*>(reinterpret_cast<uint64_t>(rsdt) + sizeof(ACPISDTHeader) + (i * 4)));

		if (!hdr || *(uint32_t*)(hdr->Signature) != signature) continue;

		return hdr;
	}

	return nullptr;
}

int local_apic_count = 0;

struct LocalAPIC
{
	uint8_t type;
	uint8_t length;
	uint8_t apic_proc_id;
	uint8_t apic_id;
	uint32_t flags;
} local_apics[256];

struct IoApic
{
	uint8_t type;
	uint8_t length;
	uint8_t apic_id;
	uint8_t reserved;
	uint32_t addr;
	uint32_t gsib;
} io_apic;

int redirect_count = 0;

struct [[gnu::packed]] MADTIso
{
	uint8_t type;
	uint8_t length;
	uint8_t bus_source;
    uint8_t irq_source;
    uint32_t gsi;
    uint16_t flags;
} redirects[256];

IOAPIC* ioapic;

void ACPI::ParseACPI()
{
	Terminal::SetScreenColors(0xFFFFFFFF, 0x00000000);

	printf("ACPI table is at 0x%lx\n", rsdp_req.response->address);

	RSDPointer* rsdp = (RSDPointer*)rsdp_req.response->address;

	if (strncmp(rsdp->Signature, "RSD PTR ", 8))
	{
		printf("ERROR: ACPI RSDP signature does not match!\n");
		Arch::Halt();
	}

	rsdt = (ACPISDTHeader*)(uint64_t)rsdp->RsdtAddress;

	printf("RSDT is at 0x%lx\n", rsdp->RsdtAddress);

	MADTHeader* madt = (MADTHeader*)FindTable(0x43495041, 0);

	printf("MADT is at 0x%lx\n", madt);
	
	for (uint8_t* madt_ptr = (uint8_t*)madt->entries_begin; (uintptr_t)madt_ptr < (uintptr_t)madt + madt->sdt.Length; madt_ptr += *(madt_ptr + 1))
	{
		switch (*(madt_ptr))
		{
		case 0:
			local_apics[local_apic_count] = *(LocalAPIC*)madt_ptr;
			cpus[local_apic_count].apic_id = local_apics[local_apic_count].apic_proc_id;
			printf("Found local APIC %ld with id %d\n", local_apic_count, cpus[local_apic_count].apic_id);
			local_apic_count++;
			break;
		case 1:
			io_apic = *(IoApic*)madt_ptr;
			printf("Found IOAPIC %d, at 0x%lx\n", io_apic.apic_id, io_apic.addr);
			ioapic = new IOAPIC(io_apic.addr);
			break;
		case 2:
			redirects[redirect_count] = *(MADTIso*)madt_ptr;
			printf("Found IRQ redirect for IOAPIC (%d -> %d)\n", redirects[redirect_count].irq_source, redirects[redirect_count].gsi);
			redirect_count++;
			break;
		case 4:
			break;
		default:
			printf("MADT entry type %d\n", *(madt_ptr));
		}
	}

	for (int i = 0; i < redirect_count; i++)
	{
		ioapic->RedirectIRQ(redirects[i].gsi, redirects[i].irq_source+0x20, 0);
	}
}

int ACPI::GetCPUCount()
{
	return local_apic_count;
}
