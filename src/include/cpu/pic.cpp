#include "pic.h"

void PIC::RemapAndDisable()
{
    Utils::outb(0x20, 0x11);
    Utils::outb(0xA0, 0x11);
    Utils::outb(0x21, 0x20);
    Utils::outb(0xA1, 0x28);
    Utils::outb(0x21, 4);
    Utils::outb(0xA1, 2);

    Utils::outb(0x21, 0x01);
    Utils::outb(0xA1, 0x01);

    // Mask all interrupts
    Utils::outb(0x21, 0xff);
    Utils::outb(0xA1, 0xff);
}