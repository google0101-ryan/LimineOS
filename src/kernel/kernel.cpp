#include <include/limine.h>
#include <include/util.h>
#include <include/screen.h>
#include <include/cpu/gdt.h>
#include <include/cpu/idt.h>
#include <include/cpu/features.h>
#include <include/cpu/pic.h>
#include <include/mm/pmm.h>
#include <include/mm/vmm.h>
#include <include/mm/liballoc.h>
#include <include/dev/acpi.h>
#include <include/cpu/spinlock.h>
#include <include/dev/apic.h>
#include <include/drivers/pata.h>
#include <include/drivers/igpu.h>

#define KERNEL_VERSION "1.0.0"
#define KERNEL_CODENAME "Hudson"

uint8_t kernel_stack[0x4000];

uint64_t cpus_arrived = 1;
spinlock cpu_lock;

void start_ap(limine_smp_info* info)
{
    GDT::SetupGdt(info->processor_id);

    VirtualMemory::SwapToKernelPT();

    printf("Started processor %d\n", info->processor_id);

    cpu_lock.lock();
    cpus_arrived++;
    cpu_lock.unlock();

    Utils::HaltCatchFire();
}

volatile limine_smp_request smp_req = 
{
    .id = LIMINE_SMP_REQUEST,
    .revision = 0,
    .response = nullptr,
    .flags = 0
};

extern "C" void kernel_entry [[gnu::noreturn]]()
{
    Screen::Initialize();

    printf("Succesfully initialized framebuffer, early printf enabled, output redirected to com0\n");
    printf("Kernel version %s (%s)\n", KERNEL_VERSION, KERNEL_CODENAME);

    IDT::Initialize();

    printf("IDT setup done...         \t\t\t\t[OK]\n");

    PIC::RemapAndDisable();

    printf("Remapped PIC interrupts (0 -> 32)\n");

    printf("Initializing physical memory\n");

    PhysicalMemory::Initialize();

    GDT::SetupGdt(0);

    printf("GDT setup done...         \t\t\t\t[OK]\n");

    printf("Initializing virtual memory\n");

    VirtualMemory::Initialize();

    printf("Parsing ACPI tables\n");

    ACPI::ParseTables();

    printf("Initializing APIC\n");

    LAPIC::Initialize();
    IOAPIC::Initialize();

    printf("Starting all cpus\n");

    for (uint64_t i = 0; i < smp_req.response->cpu_count; i++)
    {
        auto& cpu = smp_req.response->cpus[i];

        if (cpu->lapic_id == smp_req.response->bsp_lapic_id) continue;

        cpu->goto_address = start_ap;
    }

    while (cpus_arrived < smp_req.response->cpu_count)
        __builtin_ia32_pause();

    printf("All CPUs arrived\n");

    IntelGpu::Initialize();

    Utils::HaltCatchFire();
}