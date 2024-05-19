#include <include/dev/apic.h>
#include <include/util.h>
#include <include/limine.h>
#include <include/dev/acpi.h>
#include <include/drivers/hpet.h>
#include <include/cpu/idt.h>
#include <include/sched/scheduler.h>
#include <include/screen.h>

extern uint64_t lapic_addr;

extern limine_hhdm_request hhdm_req;

void LAPIC::Initialize()
{
    Utils::wrmsr(0x1B, Utils::rdmsr(0x1B) | 0x800);

    WriteReg(0xF0, 0x1FF); // Enable LAPIC, set spurious vector to 255
}

SavedRegs* HandleTick(SavedRegs* r)
{
    Scheduler::Schedule(r);
    LAPIC::EOI();
    return r;
}

void LAPIC::InitTimer(bool is_bsp)
{
    WriteReg(0x3E0, 0x3);
    WriteReg(0x380, 0xFFFFFFFF);

    msleep(10);

    WriteReg(0x320, 0x10000);

    uint32_t ticksIn10ms = 0xFFFFFFFF - ReadReg(0x390);

    WriteReg(0x320, 0x20020);
    WriteReg(0x3E0, 0x3);
    WriteReg(0x380, ticksIn10ms);

    if (is_bsp)
        IDT::AddHandler(32, HandleTick);

    printf("LAPIC timer initialized\n");

    // IOAPIC::WriteReg()
}

void LAPIC::WriteReg(uint32_t index, uint32_t val)
{
    *(volatile uint32_t*)(lapic_addr + hhdm_req.response->offset + index) = val;
}

uint32_t LAPIC::ReadReg(uint32_t index)
{
    return *(volatile uint32_t *)(lapic_addr + hhdm_req.response->offset + index);
}

void LAPIC::EOI()
{
    WriteReg(0xB0, 0);
}

uint32_t IOAPIC::ReadReg(uint32_t index)
{
    *(uint32_t*)(0xfec00000ULL + hhdm_req.response->offset) = index;
    return *(uint32_t*)(0xfec00010ULL + hhdm_req.response->offset);
}

void IOAPIC::WriteReg(uint32_t index, uint32_t data)
{
    *(uint32_t*)(0xfec00000ULL + hhdm_req.response->offset) = index;
    *(uint32_t*)(0xfec00010ULL + hhdm_req.response->offset) = data;
}

void RedirectGSI(uint32_t gsi, uint8_t irq)
{
    printf("IOAPIC redirect %d -> %d\n", irq, gsi);
    uint32_t low_index = 0x10 + gsi * 2;
    uint32_t high_index = low_index + 1;

    uint32_t high = IOAPIC::ReadReg(high_index);

    high &= ~0xFF000000;
    high |= IOAPIC::ReadReg(0) << 24;
    IOAPIC::WriteReg(high_index, high);

    uint32_t low = IOAPIC::ReadReg(low_index);

    low &= ~(1 << 16);
    low &= ~(1 << 11);
    low &= ~0x700;
    low &= ~0xFF;
    low |= irq;

    IOAPIC::WriteReg(low_index, low);
}

void IOAPIC::Initialize()
{
    for (size_t i = 0; i < numGSIs; i++)
        RedirectGSI(gsis[i].gsi_source, gsis[i].irq_source);
}