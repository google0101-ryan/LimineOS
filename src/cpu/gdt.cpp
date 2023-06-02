#include <include/screen.h>
#include <include/cpu/gdt.h>
#include <include/limine.h>
#include <include/mm/pmm.h>
#include <include/cpu/cpu.h>
#include <include/cpu/spinlock.h>

extern limine_hhdm_request hhdm_req;

volatile limine_stack_size_request stack_size = 
{
    .id = LIMINE_STACK_SIZE_REQUEST,
    .revision = 0,
    .response = nullptr,
    .stack_size = 0
};

static volatile GDT::GDT gdt;
static volatile GDT::GDTR gdtr;
static GDT::TSS tss[256];

extern "C" void LoadGdt(uint64_t gdt_ptr);

extern uint8_t* kernel_stack;

spinlock gdt_lock;

void GDT::SetupGdt(int processor_id)
{
    gdt_lock.lock();

    tss[processor_id].rsp[0] = (uint64_t)(PhysicalMemory::alloc(0x4000 / 4096) + 0x4000) + hhdm_req.response->offset;
    tss[processor_id].iopb = sizeof(TSS);

    printf("TSS kernel stack is now at 0x%x\n", tss[processor_id].rsp[0] - 0x4000);

    gdt.null.access = 0;
    gdt.null.base0 = 0;
    gdt.null.base1 = 0;
    gdt.null.base2 = 0;
    gdt.null.flags = 0;
    gdt.null.limit0 = 0;
    gdt.null.limit1 = 0;

    gdt.kernel_code.access = 0x9A;
    gdt.kernel_code.base0 = 0;
    gdt.kernel_code.base1 = 0;
    gdt.kernel_code.base2 = 0;
    gdt.kernel_code.flags = 0xA;
    gdt.kernel_code.limit0 = 0xFFFF;
    gdt.kernel_code.limit1 = 0xFF;

    gdt.kernel_data.access = 0x92;
    gdt.kernel_data.base0 = 0;
    gdt.kernel_data.base1 = 0;
    gdt.kernel_data.base2 = 0;
    gdt.kernel_data.flags = 0xC;
    gdt.kernel_data.limit0 = 0xFFFF;
    gdt.kernel_data.limit1 = 0xFF;

    gdt.user_code.access = 0xFA;
    gdt.user_code.base0 = 0;
    gdt.user_code.base1 = 0;
    gdt.user_code.base2 = 0;
    gdt.user_code.flags = 0xA;
    gdt.user_code.limit0 = 0xFFFF;
    gdt.user_code.limit1 = 0xFF;

    gdt.user_data.access = 0xF2;
    gdt.user_data.base0 = 0;
    gdt.user_data.base1 = 0;
    gdt.user_data.base2 = 0;
    gdt.user_data.flags = 0xC;
    gdt.user_data.limit0 = 0xFFFF;
    gdt.user_data.limit1 = 0xFF;

    uint64_t tss_base = (uint64_t)&tss[processor_id];
    uint16_t tss_size = sizeof(tss[processor_id]);

    gdt.tss.access = 0x89;
    gdt.tss.flags = 0x00;
    gdt.tss.rsv = 0;
    gdt.tss.base0 = (tss_base & 0xFFFF);
    gdt.tss.base1 = (tss_base >> 16) & 0xFF;
    gdt.tss.base2 = (tss_base >> 24) & 0xFF;
    gdt.tss.base3 = (tss_base >> 32);
    gdt.tss.limit0 = (tss_size & 0xffff);
    gdt.tss.limit1 = 0;

    gdtr.offset = (uint64_t)&gdt;
    gdtr.size = sizeof(GDT)-1;

    LoadGdt((uint64_t)&gdtr);

    asm volatile("ltr %0" :: "r"((uint16_t)0x28));

    cpus[processor_id].tss = &tss[processor_id];

    gdt_lock.unlock();
}