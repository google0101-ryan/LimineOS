#include <include/cpu/features.h>
#include <include/screen.h>
#include <include/util.h>
#include <cpuid.h>
#include <cstdint>

extern "C" void EnableAvx();

void Features::DetectAndInit()
{
    uint32_t eax, ebx, ecx, edx;
    __cpuid(1, eax, ebx, ecx, edx);

    if (!(edx & (1 << 25)))
    {
        printf("PANIC! Essential CPU feature SSE not detected!\n");
        Utils::HaltCatchFire();
    }

    if (!(edx & (1 << 26)))
    {
        printf("PANIC! Essential CPU feature SSE2 not detected!\n");
        Utils::HaltCatchFire();
    }

    if (!(ecx & (1 << 0)))
    {
        printf("PANIC! Essential CPU feature SSE3 not detected!\n");
        Utils::HaltCatchFire();
    }
    
    __cpuid_count(0x07, 0x00, eax, ebx, ecx, edx);

    // printf("0x%x\n", ebx);

    // if (!(ebx & (1 << 5)))
    // {
    //     printf("PANIC! Essential CPU feature AVX2 not detected!\n");
    //     Utils::HaltCatchFire();
    // }

    uint32_t cr0 = Utils::ReadCr0();
    cr0 &= 0xFFFFFFFB;
    cr0 |= 0x2;
    Utils::WriteCr0(cr0);

    uint32_t cr4 = Utils::ReadCr4();
    cr4 |= (3 << 9) | (1 << 18);
    Utils::WriteCr4(cr4);

    printf("SSE enabled\n");

    asm volatile("fninit\n");

    printf("FPU enabled\n");

    // EnableAvx();
}