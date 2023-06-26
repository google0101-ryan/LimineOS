#include <include/cpu/cpu.h>
#include <include/util.h>

CPU *GetCurCPU()
{
    return (CPU*)Utils::rdmsr(0xC0000102);
}

CPU cpus[256];