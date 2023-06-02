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

namespace ACPI
{

void ParseTables(); // Parses ACPI tables such as the APIC stuff (we need that to initialize the APIC)

}