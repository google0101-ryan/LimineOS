#pragma once

#include <stdint.h>
#include <stddef.h>

struct GSIRedirect
{
    uint8_t irq_source;
    uint32_t gsi_source;
};

extern GSIRedirect gsis[256];
extern size_t numGSIs;

struct [[gnu::packed]] SDTHdr
{
    char sig[4];
    uint32_t len;
    uint8_t rev;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_rev;
};

namespace ACPI
{

void* FindTable(const char* sig);
void ParseTables(); // Parses ACPI tables such as the APIC stuff (we need that to initialize the APIC)

}