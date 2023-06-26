#include <include/dev/acpi.h>
#include <include/limine.h>
#include <include/screen.h>
#include <include/util.h>

extern limine_hhdm_request hhdm_req;

limine_rsdp_request rsdp = 
{
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0,
    .response = nullptr
};

struct RSDP
{
    char sig[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t rev;
    uint32_t rsdt_addr;
};

struct RSDT
{
    SDTHdr hdr;
    uint32_t sdts[];
} *rsdt;

void* ACPI::FindTable(const char* sig)
{
    for (size_t i = 0; i < (rsdt->hdr.len - sizeof(SDTHdr)) / 4; i++)
    {
        SDTHdr* hdr = (SDTHdr*)(((uint64_t)rsdt->sdts[i]) + hhdm_req.response->offset);
        if (!strncmp(hdr->sig, sig, 4))
        {
            return (void*)hdr;
        }
    }

    return nullptr;
}

uint32_t lapic_addr = 0;

GSIRedirect gsis[256];

size_t numGSIs = 0;

void ParseAPIC()
{
    uint64_t addr = (uint64_t)ACPI::FindTable("APIC");
    SDTHdr* madt = (SDTHdr*)addr;

    if (!madt)
    {
        printf("ERROR: No MADT table present\n");
        return;
    }
    
    lapic_addr = *(uint32_t*)(addr + 0x24);

    size_t pos = 0x2C;
    while (pos < madt->len)
    {
        uint8_t entryType = *(uint8_t*)(addr + pos);
        uint8_t recordLen = *(uint8_t*)(addr + pos + 1);
        
        switch (entryType)
        {
        case 0:
        {
            uint8_t processor_id = *(uint8_t*)(addr + pos + 2);
            uint8_t apic_id = *(uint8_t*)(addr + pos + 3);
            uint8_t flags = *(uint8_t*)(addr + pos + 4);
            break;
        }
        case 1:
        {
            uint8_t ioapic_id = *(uint8_t*)(addr + pos + 2);
            uint32_t ioapic_addr = *(uint32_t*)(addr + pos + 4);
            uint32_t gsi_base = *(uint32_t*)(addr + pos + 8);
            break;
        }
        case 2:
        {
            uint8_t bus_source = *(uint8_t*)(addr + pos + 2);
            uint8_t irq_source = *(uint8_t*)(addr + pos + 3);
            uint32_t gsi_source = *(uint32_t*)(addr + pos + 4);
            gsis[numGSIs++] = {.irq_source = irq_source, .gsi_source = gsi_source};
            break;
        }
        }

        pos += recordLen;
    }
}

void ACPI::ParseTables()
{
    auto rsdp_ptr = rsdp.response->address;

    printf("RSDP is at 0x%x\n", rsdp_ptr);

    volatile RSDP* rsdp = (volatile RSDP*)rsdp_ptr;

    if (strncmp((const char*)rsdp->sig, "RSD PTR ", 8))
    {
        printf("RSDP signature doesn't match!\n");
        Utils::HaltCatchFire();
    }

    rsdt = (RSDT*)(rsdp->rsdt_addr+hhdm_req.response->offset);

    ParseAPIC();
}