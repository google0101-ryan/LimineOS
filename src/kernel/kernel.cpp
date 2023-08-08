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
#include <include/cpu/cpu.h>
#include <include/sched/scheduler.h>
#include <include/drivers/hpet.h>
#include <include/fs/vfs.h>

#define KERNEL_VERSION "1.0.0"
#define KERNEL_CODENAME "Hudson"

uint8_t kernel_stack[0x4000];

uint64_t cpus_arrived = 1;
spinlock cpu_lock;

void start_ap(limine_smp_info* info)
{
    cpu_lock.lock();
    GDT::SetupGdt(info->processor_id);
    IDT::Initialize();

    VirtualMemory::SwapToKernelPT();

    Utils::wrmsr(0xC0000102, (uint64_t)&cpus[info->processor_id]);
    cpus[info->processor_id].processor_id = info->processor_id;

    printf("Started processor %d\n", info->processor_id);

    LAPIC::Initialize();
    LAPIC::InitTimer(false);

    cpus_arrived++;
    cpu_lock.unlock();

    Utils::Halt();
}

volatile limine_smp_request smp_req = 
{
    .id = LIMINE_SMP_REQUEST,
    .revision = 0,
    .response = nullptr,
    .flags = 0
};

void BspKernelThread()
{
    printf("Entered BSP kernel thread\n");

    printf("Initializing VFS...\n");

    VFS::Initialize();

    for (;;)
        asm volatile("hlt");
}

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

    printf("Initializing scheduler\n");

    Scheduler::Initialize();

    Scheduler::AddThread((uint64_t)BspKernelThread, false); // Make sure BSP has at least one thread in its queue

    HPET::SetupHPET(); // We use the HPET to initialize all LAPIC timers

    asm volatile("cli");

    LAPIC::InitTimer(true);

    printf("System init done.\n");

    printf("Starting all cpus\n");

    for (uint64_t i = 0; i < smp_req.response->cpu_count; i++)
    {
        auto& cpu = smp_req.response->cpus[i];

        if (cpu->lapic_id == smp_req.response->bsp_lapic_id)
        { 
            cpus[cpu->processor_id].processor_id = cpu->processor_id;
            Utils::wrmsr(0xC0000102, (uint64_t)&cpus[cpu->processor_id]);
            continue;
        }

        // cpu->goto_address = start_ap;
    }

    // while (cpus_arrived < smp_req.response->cpu_count)
    //     __builtin_ia32_pause();

    printf("All CPUs arrived\n");

    asm volatile("sti");

    for (;;)
        asm volatile("hlt");
}