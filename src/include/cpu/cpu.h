#pragma once

namespace GDT
{
struct TSS;
}

struct CPU
{
    GDT::TSS* tss;
};

extern CPU cpus[256];