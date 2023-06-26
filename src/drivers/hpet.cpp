#include <include/drivers/hpet.h>
#include <include/dev/acpi.h>
#include <include/screen.h>
#include <include/limine.h>

struct [[gnu::packed]] HpetTable
{
    SDTHdr hdr;
    uint8_t hardware_rev_id;
    uint8_t comparator_count : 5;
    uint8_t counter_size : 1;
    uint8_t reserved : 1;
    uint8_t legacy_replacement : 1;
    uint16_t pci_vendor_id;
    uint8_t address_space_id;
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t reserved2;
    uint64_t address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
} *hpetTable;

struct [[gnu::packed]] HpetRegs
{
    uint64_t capabilities;
    uint64_t rsvd0;
    uint64_t general_config;
    uint64_t rsvd1;
    uint64_t interrupt_status;
    uint64_t rsvd2;
    uint64_t rsvd3[24];
    volatile uint64_t counter_value;
    uint64_t unused;
} *hpetRegs;


static_assert(sizeof(HpetTable)-sizeof(SDTHdr) == 20);

void HPET::SetupHPET()
{
    hpetTable = (HpetTable*)ACPI::FindTable("HPET");

    if (!hpetTable)
        printf("ERROR: Could not find HPET table (0x%x)\n", hpetTable);

    printf("HPET is at 0x%x\n", hpetTable->address);

    hpetTable->address += hhdm_req.response->offset;

    hpetRegs = (HpetRegs*)hpetTable->address;

    hpetRegs->counter_value = 0;
    
    hpetRegs->general_config = 1;
}

void msleep(size_t ms)
{
    uint32_t period = hpetRegs->capabilities >> 32;

    volatile size_t ticks = hpetRegs->counter_value + (ms * (1000000000000 / period));

    while (hpetRegs->counter_value < ticks)
        asm volatile("pause");
}