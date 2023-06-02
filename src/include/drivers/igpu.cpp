#include "igpu.h"
#include "igpu_regs.h"

#include <include/util.h>
#include <include/screen.h>
#include <include/mm/pmm.h>
#include <include/mm/vmm.h>

using namespace Utils;

#define PCI_DEVICE_GPU 0, 2, 0
#define IGFX_IO_VADDR 0xffffc00000000000
#define DISPLAY_COUNT 3

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

inline void WriteIgpu32(uint32_t reg, uint32_t data)
{
    iobase[reg / 4] = data;
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

uint64_t GetStolenMemSize(uint32_t size_code)
{
    switch (size_code)
    {
    case 0x01:
        return 32*1024*1024;
    case 0x02:
        return 64*1024*1024;
    }
}

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
}