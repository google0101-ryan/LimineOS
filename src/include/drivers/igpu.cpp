#include "igpu.h"
#include "igpu_regs.h"

#include <include/util.h>
#include <include/screen.h>
#include <include/mm/pmm.h>
#include <include/mm/vmm.h>
#include <include/limine.h>

extern limine_hhdm_request hhdm_req;

using namespace Utils;

#define PCI_DEVICE_GPU 0, 2, 0
#define IGFX_IO_VADDR 0xffffc00000000000
#define IGFX_STOLENMEM_VADDR 0xffffc00000400000
#define DISPLAY_COUNT 3

#define RCS_HWS_PGA                     0x04080
#define HWS_PGA(id) 					(RCS_HWS_PGA + ((id) << 8))
#define RING_BUFFER_TAIL(id)			(RCS_RINGBUF_BUFFER_TAIL + ((id) << 8))
#define RING_BUFFER_HEAD(id)			(RCS_RINGBUF_BUFFER_HEAD + ((id) << 8))
#define RING_BUFFER_START(id)			(RCS_RINGBUF_BUFFER_START + ((id) << 8))
#define RING_BUFFER_CTL(id)				(RCS_RINGBUF_BUFFER_CTRL + ((id) << 8))

uint16_t ReadPciRegister16(int bus, int device, int func, uint8_t offs)
{
    outd(0xCF8, 0x80000000 | (bus << 16) | (device << 11) | (func << 8) | offs);
    return inw(0xCFC);
}

uint32_t ReadPciRegister32(int bus, int device, int func, uint8_t offs)
{
    outd(0xCF8, 0x80000000 | (bus << 16) | (device << 11) | (func << 8) | offs);
    return ind(0xCFC);
}

uint64_t ReadPciRegister64(int bus, int device, int func, uint8_t offs)
{
    return (uint64_t)ReadPciRegister32(bus, device, func, offs+4) << 32 | ReadPciRegister32(bus, device, func, offs);
}

void WritePciRegister16(int bus, int device, int func, uint8_t offs, uint16_t data)
{
    outd(0xCF8, 0x80000000 | (bus << 16) | (device << 11) | (func << 8) | offs);
    outw(0xCFC, data);
}

void WritePciRegister32(int bus, int device, int func, uint8_t offs, uint32_t data)
{
    outd(0xCF8, 0x80000000 | (bus << 16) | (device << 11) | (func << 8) | offs);
    outd(0xCFC, data);
}

volatile uint32_t* iobase;
volatile uint32_t* gttAddr;

inline void WriteIgpu32(uint32_t reg, uint32_t data)
{
    iobase[reg / 4] = data;
}

inline void WriteIgpu64(uint32_t reg, uint64_t data)
{
    *(uint64_t*)&iobase[reg / 4] = data;
}

uint32_t ReadIgpu32(uint32_t reg)
{
    return iobase[reg / 4];
}

struct DisplayInfo
{
    DisplayType type;
    bool isPresent = false;
    bool backlightEnabled;
    uint32_t pipe_assignment;
    uint32_t max_backlight_val;
};

static const uint32_t GMS_TO_SIZE[] =
{
      0 * 1024*1024,     // GMS_0MB
     32 * 1024*1024,     // GMS_32MB_1
     64 * 1024*1024,     // GMS_64MB_1
     96 * 1024*1024,     // GMS_96MB_1
    128 * 1024*1024,     // GMS_128MB_1
     32 * 1024*1024,     // GMS_32MB
     48 * 1024*1024,     // GMS_48MB
     64 * 1024*1024,     // GMS_64MB
    128 * 1024*1024,     // GMS_128MB
    256 * 1024*1024,     // GMS_256MB
     96 * 1024*1024,     // GMS_96MB
    160 * 1024*1024,     // GMS_160MB
    224 * 1024*1024,     // GMS_224MB
    352 * 1024*1024,     // GMS_352MB
    448 * 1024*1024,     // GMS_448MB
    480 * 1024*1024,     // GMS_480MB
    512 * 1024*1024,     // GMS_512MB
};

uint32_t stolenMemSize, stolenMemBase;
uint32_t gttMemSize, gttEntryCount, gttMappableEntries;
uint64_t arpertureBar;

static void PciReadBar(int bus, int device, int func, int index, uint32_t* address, uint32_t* mask)
{
    uint32_t reg = 0x10 + index * sizeof(uint32_t);

    *address = ReadPciRegister32(bus, device, func, reg);
    WritePciRegister32(bus, device, func, reg, 0xffffffff);
    *mask = ReadPciRegister32(bus, device, func, reg);

    WritePciRegister32(bus, device, func, reg, *address);
}

struct PciBar
{
    uint64_t base, size;
    bool is_io;
};

void PciGetBar(PciBar* bar, int bus, int dev, int func, int index)
{
    uint32_t addressLow;
    uint32_t maskLow;
    PciReadBar(bus, dev, func, index, &addressLow, &maskLow);

    if (addressLow & 0x04)
    {
        uint32_t addressHigh;
        uint32_t maskHigh;
        PciReadBar(bus, dev, func, index+1, &addressHigh, &maskHigh);

        bar->base = ((uintptr_t)addressHigh << 32) | (addressLow & ~0xf);
        bar->size = ~(((uint64_t)maskHigh << 32) | (maskLow & ~0xf)) + 1;
        bar->is_io = false;
    }
    else if (addressLow & 1)
    {
        bar->base = (uint16_t)(addressLow & ~3);
        bar->size = (uint16_t)(~(maskLow & ~0x3) + 1);
        bar->is_io = true;
    }
    else
    {
        bar->base = (addressLow & ~0xf);
        bar->size = ~(maskLow & ~0xf) + 1;
        bar->is_io = false;
    }
}

struct GfxObject
{
    uint8_t* cpuAddr;
    uint64_t gfxAddr;
};

struct GfxMemRange
{
    uint64_t base;
    uint64_t top;
    uint64_t current;
};

struct GfxMemManager
{
    GfxMemRange vram;
    GfxMemRange shared;
    GfxMemRange private_;

    uint8_t* gfxMemBase;
    uint8_t* gfxMemNext;
} mem_manager;

enum GfxRingId
{
	RING_RCS,
	RING_VCS,
	RING_BCS,
	RING_COUNT,
};

struct GfxRing
{
	GfxRingId id;
	GfxObject cmdStream;
	GfxObject statusPage;
	uint8_t* tail;
} csRing;

void GfxInitMemManager()
{
    mem_manager.vram.base = 0;
    mem_manager.vram.current = 0;
    mem_manager.vram.top = stolenMemSize;

    mem_manager.shared.base = stolenMemSize;
    mem_manager.shared.current = mem_manager.shared.base;
    mem_manager.shared.top = gttMappableEntries << 12;

    mem_manager.private_.base = gttMappableEntries << 12;
    mem_manager.private_.current = mem_manager.private_.base;
    mem_manager.private_.top = ((uint64_t)gttMappableEntries) << 12;

    for (uint8_t fenceNum = 0; fenceNum < 16; fenceNum++)
    {
        WriteIgpu64(0x100000 + fenceNum * sizeof(uint64_t), 0);
    }

    mem_manager.gfxMemBase = (uint8_t*)(arpertureBar + 0x400800 + hhdm_req.response->offset); // Avoid overwriting our 1366x768 32BPP framebuffer at the base of our shared memory
    mem_manager.gfxMemNext = mem_manager.gfxMemBase + 4 * (1 << 12);
}

bool GfxAlloc(GfxObject* obj, uint32_t size, uint32_t align)
{
    uint8_t* cpuAddr = mem_manager.gfxMemNext;
    uintptr_t offset = (uintptr_t)cpuAddr & (align - 1);
    if (offset)
    {
        cpuAddr += align - offset;
    }

    mem_manager.gfxMemNext = cpuAddr + size;
    obj->cpuAddr = cpuAddr;
    obj->gfxAddr = cpuAddr - mem_manager.gfxMemBase;
    return true;
}

void EnterForceWake()
{
    int trys = 0;
    int forceWakeAck;
    do
    {
        ++trys;
        forceWakeAck = ReadIgpu32(0x130040) & 1;
    } while (forceWakeAck != 0 && trys < 10);

    WriteIgpu32(0xA188, (1 << 16) | 1);
    ReadIgpu32(0xA180);

    do
    {
        ++trys;
        forceWakeAck = ReadIgpu32(0x130040) & 1;
        printf("Waiting for force ack to be set: Try=%d - Ack=0x%x\n", trys, forceWakeAck);
    } while (forceWakeAck == 0);
}

void ExitForceWake()
{
    WriteIgpu32(0xA188, (1 << 16));
    ReadIgpu32(0xA180);
}

GfxObject surface, cursor, renderContext;

void IntelGpu::Initialize()
{
    printf("Command register is 0x%x\n", ReadPciRegister16(PCI_DEVICE_GPU, 0x04));

    WritePciRegister16(PCI_DEVICE_GPU, 0x04, ReadPciRegister16(PCI_DEVICE_GPU, 0x04) | (1 << 2)); // Enable bus mastering

    printf("Command register is 0x%x\n", ReadPciRegister16(PCI_DEVICE_GPU, 0x04));

    uint64_t io_base = ReadPciRegister64(PCI_DEVICE_GPU, 0x10) & ~7; // Clear bit 0, which indicates IO/MMIO
    for (int i = 0; i < ((4*1024*1024) / 4096); i++)
        VirtualMemory::MapPage(VirtualMemory::kernelDir, IGFX_IO_VADDR+(i*4096), io_base+(i*4096), PageFlags::present | PageFlags::readwrite);
    
    printf("I/O base is at physical address 0x%x\n", ReadPciRegister64(PCI_DEVICE_GPU, 0x10));

    iobase = (uint32_t*)IGFX_IO_VADDR;
    gttAddr = (uint32_t*)(IGFX_IO_VADDR+(2*1024*1024));

    bool is_lvds_connected = ReadIgpu32(LVDS_CTL) & (1 << 1);

    if (!is_lvds_connected)
    {
        printf("ERROR: LVDS is disconnected\n");
        return;
    }

    DisplayInfo display;
    display.type = DisplayType_LVDS;
    display.isPresent = true;
    
    printf("Enabling backlight\n");

    uint32_t val = ReadIgpu32(PP_CONTROL);
    val |= (1 << 2);
    WriteIgpu32(PP_CONTROL, val);
    display.backlightEnabled = true;

    printf("Backlight enabled\n");

    val = ReadIgpu32(BLC_PWM_CTL);
    val |= (1 << 31);
    display.pipe_assignment = (val >> 29) & 1;
    WriteIgpu32(BLC_PWM_CTL, val);

    printf("PWM enabled\n");

    uint32_t pwm_mod_freq = ReadIgpu32(SBLC_PWM_CTL2);
    display.max_backlight_val = pwm_mod_freq >> 16;

    WriteIgpu32(BLC_PWM_DATA, display.max_backlight_val / 2);

    printf("Backlight brightness set\n");

    uint16_t ggc = ReadPciRegister16(PCI_DEVICE_GPU, 0x50);
    uint32_t bdsm = ReadPciRegister32(PCI_DEVICE_GPU, 0x5C);

    uint32_t gms = (ggc >> 3) & 0x1F;
    stolenMemSize = GMS_TO_SIZE[gms];

    uint32_t ggms = (ggc >> 8) & 0x3;

    switch (ggms)
    {
    case 0:
        gttMemSize = 0;
        break;
    case 1:
        gttMemSize = 1*1024*1024;
        break;
    case 2:
        gttMemSize = 2*1024*1024;
        break;
    default:
        gttMemSize = -1;
        break;
    }

    PciBar aperture;
    PciGetBar(&aperture, PCI_DEVICE_GPU, 2);
    arpertureBar = aperture.base;

    printf("GMADR: 0x%x (%u MB)\n", aperture.base, aperture.size / (1024*1024));

    stolenMemBase = bdsm & 0xFFF00000;

    gttEntryCount = gttMemSize / sizeof(uint32_t);
    gttMappableEntries = aperture.size >> 12;
    
    printf("GTT Config:\n");
    printf("\tStolen mem base:      0x%x\n", stolenMemBase);
    printf("\tStolen mem size:      0x%x\n", stolenMemSize);
    printf("\tGTT mem size:         0x%x\n", gttMemSize);
    printf("\tGTT total entries:    0x%x\n", gttEntryCount);
    printf("\tGTT mappable entries: 0x%x\n", gttMappableEntries);

    GfxInitMemManager();
	
	// Disable VGA mode
	outb(0x3c4, 0x01);
	outb(0x3c5, inb(0x3c5) | (1 << 5));
	WriteIgpu32(0x41000, (1 << 31));

	printf("VGA plane disabled\n");

	uint32_t surfaceSize = 16 * 1024 * 1024;
	GfxAlloc(&surface, surfaceSize, 0x40000);
	memset(surface.cpuAddr, 0x77, 720*400*4);

	uint32_t cursorSize = 64*64*sizeof(uint32_t) + 8 * 1024;
	GfxAlloc(&cursor, cursorSize, 64*1024);

	uint32_t csMemSize = 4*1024;
	GfxAlloc(&csRing.cmdStream, csMemSize, 4*1024);
	GfxAlloc(&csRing.statusPage, csMemSize, 4*1024);

	csRing.id = RING_RCS;
	csRing.tail = csRing.cmdStream.cpuAddr;
	memset(csRing.statusPage.cpuAddr, 0, 4*1024);

	GfxAlloc(&renderContext, 4*1024, 4*1024);

	EnterForceWake();
	{
		WriteIgpu32(HWS_PGA(csRing.id), csRing.statusPage.gfxAddr);
		WriteIgpu32(RING_BUFFER_TAIL(csRing.id), 0);
		WriteIgpu32(RING_BUFFER_HEAD(csRing.id), 0);
		WriteIgpu32(RING_BUFFER_START(csRing.id), csRing.cmdStream.gfxAddr);
		WriteIgpu32(RING_BUFFER_CTL(csRing.id), (0 << 12) | 1);
	}
	ExitForceWake();

	uint32_t* cmd = (uint32_t*)csRing.tail;
	*cmd++ = MI_INSTR(0x00, 0) | (1 << 22) | 0xaa55;
	csRing.tail = (uint8_t*)cmd;
	uint32_t tailIndex = (csRing.tail - csRing.cmdStream.cpuAddr);

	EnterForceWake();
	{
		WriteIgpu32(RING_BUFFER_TAIL(csRing.id), tailIndex);
	}
	ExitForceWake();

	printf("0x%08x\n", ReadIgpu32(NOPID));
}