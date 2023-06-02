#pragma once

#include <stdint.h>

namespace GDT
{

struct [[gnu::packed]] GDTR
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
    uint8_t limit1 : 4;
    uint8_t flags : 4;
    uint8_t base2;
};

struct [[gnu::packed]] TSSEntry
{
    uint16_t limit0;
    uint16_t base0;
    uint8_t base1;
    uint8_t access;
    uint8_t limit1 : 4;
    uint8_t flags : 4;
    uint8_t base2;
    uint32_t base3;
    uint32_t rsv;
};

struct [[gnu::packed]] TSS
{
    uint32_t rsv0;
    uint64_t rsp[3];
    uint64_t rsv1;
    uint64_t ist[7];
    uint64_t rsv2;
    uint16_t rsv3;
    uint16_t iopb;
};

struct GDT
{
    GDTEntry null;
    GDTEntry kernel_code;
    GDTEntry kernel_data;
    GDTEntry user_code;
    GDTEntry user_data;
    TSSEntry tss;
};

static_assert(sizeof(TSS) == 0x68);

static_assert(sizeof(GDTEntry) == 8);

void SetupGdt(int processor_id);

}