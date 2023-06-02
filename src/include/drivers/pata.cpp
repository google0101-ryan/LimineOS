#include "pata.h"
#include <include/util.h>
#include <include/screen.h>

using namespace Utils; // I don't want to type Utils::outb every time...

#define ATA_TIMEOUT 30000000

#define ATA_DATA 0
#define ATA_NSECTOR 2
#define ATA_SECTOR 3
#define ATA_LCYL 4
#define ATA_HCYL 5

#define ATA_DRIVE_HEAD 6

#define ATA_COMMAND 7

#define ATA_STATUS 7
#define ATA_STATUS_ERR 0x01
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_BUSY 0x80

#define ATA_DEV_CTL 0x206
#define ATA_CTL_SRST 0x04

static inline void udelay(unsigned long n)
{
    if (!n)
        return;
    asm("1: dec %%eax; jne 1b;"
        : : "a" (n * 1000));
}

static inline void mdelay(unsigned long n)
{
    while (--n)
        udelay(1000);
}

struct IDEDevice
{
    struct IDEController* controller;
    bool present;
    uint8_t position;

    bool atapi = false;

    unsigned int cylinders;
    unsigned int heads;
    unsigned int sectors;
    unsigned int capacity;
};

struct IDEController
{
    uint16_t base;
    IDEDevice devices[2];
} controllers[2];

bool WaitForDevice(IDEDevice* device, uint8_t mask, uint8_t value, unsigned long timeout)
{
    uint8_t status;
    do
    {
        status = inb(device->controller->base + ATA_STATUS);
        udelay(1);
    } while ((status & mask) != value && --timeout);
    return timeout;
}

bool ResetController(IDEDevice* device)
{
    int iobase = device->controller->base;

    outb(iobase + ATA_DEV_CTL, ATA_CTL_SRST); mdelay(2);

    if (!WaitForDevice(device, ATA_STATUS_BUSY, ATA_STATUS_BUSY, 1))
        return false;
    
    outb(iobase + ATA_DEV_CTL, 0);

    if (!WaitForDevice(device, ATA_STATUS_BUSY, 0, ATA_TIMEOUT))
        return false;
}

bool SelectDevice(IDEDevice* device)
{
    int iobase = device->controller->base;

    if ((inb(iobase + ATA_STATUS) & (ATA_STATUS_BUSY | ATA_STATUS_DRQ)))
        return false;
    
    outb(iobase + ATA_DRIVE_HEAD, 0xa0 | (device->position << 4)); udelay(1);

    if ((inb(iobase + ATA_STATUS) & (ATA_STATUS_BUSY | ATA_STATUS_DRQ)))
        return false;
    
    return true;
}

void IdentifyAtaDevice(IDEDevice* device)
{
    int iobase = device->controller->base;

    const char* pos = device->position ? "slave" : "master";
    
    device->present = false;
    uint8_t status, cl, ch, cmd;
    uint16_t info[256];

    outb(iobase + ATA_NSECTOR, 0xab);
    if (inb(iobase + ATA_NSECTOR) != 0xab)
    {
        printf("Failed to init %s drive with base = 0x%x (MISMATCH NSECTOR VALUE)\n", pos, iobase);
        return;
    }

    ResetController(device);

    if (!SelectDevice(device))
    {
        printf("Error selecting %s drive with base = 0x%x\n", pos, iobase);
        return;
    }

    if (inb(iobase + ATA_NSECTOR) == 0x01 &&
        inb(iobase + ATA_SECTOR) == 0x01)
    {
        cl = inb(iobase + ATA_LCYL);
        ch = inb(iobase + ATA_HCYL);
        status = inb(iobase + ATA_STATUS);
        if (cl == 0x14 && ch == 0xeb)
        {
            device->atapi = true;
            device->present = true;
        }
        else if (cl == 0 && ch == 0 && status != 0)
            device->present = true;
    }

    if (!device->present)
        return;

    cmd = device->atapi ? 0xA1 : 0xEC;

    outb(iobase + ATA_COMMAND, cmd); udelay(1);

    if (!WaitForDevice(device, ATA_STATUS_BUSY | ATA_STATUS_DRQ | ATA_STATUS_ERR, ATA_STATUS_DRQ, ATA_TIMEOUT))
    {
        device->present = false;
        return;
    }

    for (int i = 0; i < 256; i++)
        info[i] = inw(iobase + ATA_DATA);
    
    device->cylinders = info[1];
    device->heads = info[3];
    device->sectors = info[6];

    if ((info[49] >> 9) & 1)
        device->capacity = info[60];
    else
        device->capacity = device->heads * device->sectors * device->cylinders;
}

PATA::PATA()
{
    controllers[0].base = 0x1F0;
    controllers[0].devices[0].position = 0;
    controllers[0].devices[1].position = 1;
    
    controllers[1].base = 0x170;
    controllers[1].devices[0].position = 0;
    controllers[1].devices[1].position = 1;

    for (int i = 0; i < 2; i++)
    {
        auto controller = &controllers[i];
        
        for (int j = 0; j < 2; j++)
        {
            auto device = &controller->devices[j];
            device->controller = controller;

            IdentifyAtaDevice(device);

            if (!device->present || device->atapi)
                continue;
            
            printf("%s [%u-%u]: (%u/%u/%u - %u sectors)\n",
                    device->atapi ? "CD-ROM" : "Hard Disk",
                    i, device->position,
                    device->cylinders,
                    device->heads,
                    device->sectors,
                    device->capacity);
        }
    }
}

unsigned int PATA::Read(uint64_t block, unsigned int nblocks, void *buffer)
{
    IDEDevice* device = &controllers[0].devices[0];
    IDEController* controller = device->controller;

    uint16_t* buf = (uint16_t*)buffer;
    uint8_t sc, cl, ch, hd, cmd;
    int iobase, i;

    if (!nblocks)
        return 0;
    
    if (nblocks >= 256)
        nblocks = 256;
    
    if (block + nblocks > device->capacity)
        return 0;
    
    iobase = controller->base;

    if (!SelectDevice(device))
        return 0;

    sc = block & 0xff;
    cl = (block >> 8) & 0xff;
    ch = (block >> 16) & 0xff;
    hd = (block >> 24) & 0xf;

    outb(iobase + ATA_NSECTOR, nblocks);
    outb(iobase + ATA_SECTOR, sc);
    outb(iobase + ATA_LCYL, cl);
    outb(iobase + ATA_HCYL, ch);
    outb(iobase + ATA_DRIVE_HEAD, 0x40 | device->position << 4 | hd);
    outb(iobase + ATA_COMMAND, 0x20);

    udelay(1);

    if (!WaitForDevice(device, ATA_STATUS_BUSY, 0, ATA_TIMEOUT))
        return 0;

    if (inb(iobase + ATA_STATUS) & ATA_STATUS_ERR)
        return 0;
    
    for (i = nblocks*256; --i >= 0;)
        *buf++ = inw(iobase + ATA_DATA);
    
    return nblocks;
}
