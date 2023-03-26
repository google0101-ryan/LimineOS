#include "gdt.h"

static constexpr uint8_t GDT_NULL = 0x00;
static constexpr uint8_t GDT_CODE_64 = 0x08;
static constexpr uint8_t GDT_DATA_64 = 0x10;
static constexpr uint8_t GDT_USER_CODE_64 = 0x18;
static constexpr uint8_t GDT_USER_DATA_64 = 0x20;
static constexpr uint8_t GDT_TSS = 0x28;

extern "C" void reloadSegments();

extern volatile limine_smp_request smp_req;

struct [[gnu::packed]] GDTDescriptor
{
	uint16_t size;
	uint64_t offset;	
};

struct [[gnu::packed]] GDTEntry
{
	uint16_t limit0;
    uint16_t base0;
    uint8_t base1;
    uint8_t access;
    uint8_t granularity;
    uint8_t base2;
};

struct [[gnu::packed]] TSSEntry
{
	uint16_t length;
	uint16_t base0;
	uint8_t base1;
	uint8_t flags1;
	uint8_t flags2;
	uint8_t base2;
	uint32_t base3;
	uint32_t reserved;
};

struct [[gnu::packed, gnu::aligned(0x1000)]] GDTable
{
    GDTEntry Null;
    GDTEntry _64BitCode;
    GDTEntry _64BitData;
    GDTEntry UserCode;
    GDTEntry UserData;
    TSSEntry Tss;
};

struct [[gnu::packed]] TSS 
{
	uint32_t reserved0;
	uint64_t rsp[3];
	uint64_t reserved1;
	uint64_t ist[7];
	uint64_t reserved2;
	uint16_t reserved3;
	uint16_t iopb;
};

[[gnu::aligned(0x1000)]] GDTable defaultGdt
{
	{0x0000, 0, 0, 0x00, 0x00, 0}, // NULL
	{0x0000, 0, 0, 0x9A, 0x20, 0}, // 64-bit code
	{0x0000, 0, 0, 0x92, 0x00, 0}, // 64-bit data
	{0x0000, 0, 0, 0xFA, 0x20, 0}, // User-mode code
	{0x0000, 0, 0, 0xF2, 0x00, 0}, // User-mode data
	{0x0000, 0, 0, 0x89, 0x00, 0, 0, 0} // TSS
};

GDTDescriptor descriptor;
TSS tss[256]; // Max number of CPUs supported

void GDT::Init()
{
	descriptor.offset = reinterpret_cast<uint64_t>(&defaultGdt);
	descriptor.size = sizeof(GDTable) - 1;

	uint64_t base = reinterpret_cast<uint64_t>(&tss[0]);
	uint64_t limit = base + sizeof(tss[0]);

	defaultGdt.Tss.base0 = base;
	defaultGdt.Tss.base1 = base >> 16;
	defaultGdt.Tss.flags1 = 0x89;
	defaultGdt.Tss.flags2 = 0x00;
	defaultGdt.Tss.base2 = base >> 24;
	defaultGdt.Tss.base3 = base >> 32;
	defaultGdt.Tss.reserved = 0x00;

	asm volatile("lgdt %0" :: "m"(descriptor) : "memory");
	asm volatile("ltr %0" :: "r"(static_cast<uint16_t>(GDT_TSS)));

	reloadSegments();
}

void GDT::LoadGDT(int cpunum)
{
	uint64_t base = reinterpret_cast<uint64_t>(&tss[cpunum]);
	uint64_t limit = base + sizeof(tss[cpunum]);

	defaultGdt.Tss.base0 = base;
	defaultGdt.Tss.base1 = base >> 16;
	defaultGdt.Tss.flags1 = 0x89;
	defaultGdt.Tss.flags2 = 0x00;
	defaultGdt.Tss.base2 = base >> 24;
	defaultGdt.Tss.base3 = base >> 32;
	defaultGdt.Tss.reserved = 0x00;

	asm volatile("lgdt %0" :: "m"(descriptor) : "memory");
	asm volatile("ltr %0" :: "r"(static_cast<uint16_t>(GDT_TSS)));

	reloadSegments();
}
