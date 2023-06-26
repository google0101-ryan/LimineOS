#include <include/sched/task.h>
#include <include/util.h>
#include <include/mm/pmm.h>
#include <include/limine.h>

extern limine_kernel_file_request kernel_file;
extern limine_kernel_address_request kernel_addr;
extern limine_memmap_request mmap_req;
extern limine_hhdm_request hhdm_req;

#define DEFAULT_THREAD_STACK_SIZE (64*1024)

Thread::Thread(uint64_t entryPoint, bool is_user)
{
    memset(&ctxt, 0, sizeof(ctxt));
    ctxt.rip = entryPoint;
    ctxt.cs = is_user ? 0x1B : 0x08;
    ctxt.ss = is_user ? 0x23 : 0x10;
    ctxt.rsp = 0xffffa00000000000 + DEFAULT_THREAD_STACK_SIZE;
    ctxt.rflags = 0x202;

    uint64_t stackAddr = (uint64_t)PhysicalMemory::alloc(ALIGN_UP(DEFAULT_THREAD_STACK_SIZE, 4096));
    printf("Stack is at 0x%x\n", stackAddr+hhdm_req.response->offset);
    for (size_t i = 0; i < DEFAULT_THREAD_STACK_SIZE; i++)
        *(uint8_t*)(stackAddr+hhdm_req.response->offset+i) = 0;

    rootTable = (PML4Table*)PhysicalMemory::alloc(1);
    for (size_t i = 0; i < 4096; i++)
        *(uint8_t*)((uint64_t)rootTable+hhdm_req.response->offset+i) = 0;

    // Set up default mapping
    
    // Map the kernel
    for (size_t i = 0; i < kernel_file.response->kernel_file->size; i += 4096)
        VirtualMemory::MapPage(rootTable, kernel_addr.response->virtual_base+i, kernel_addr.response->physical_base+i, PageFlags::present | PageFlags::readwrite);
    
    // Map the framebuffer
    auto mmap = mmap_req.response->entries;
    uint64_t phyAddr = 0;
    uint64_t len = 0;

    for (size_t i = 0; i < mmap_req.response->entry_count; i++)
    {
        if (mmap[i]->type != LIMINE_MEMMAP_FRAMEBUFFER) continue;

        phyAddr = mmap[i]->base;
        len = mmap[i]->length;
        break;
    }

    for (size_t i = 0; i < len; i += 4096)
        VirtualMemory::MapPage(rootTable, reinterpret_cast<uint64_t>(Screen::GetFramebuffer())+i, phyAddr+i, PageFlags::present | PageFlags::readwrite);
    
    for (size_t i = 0; i < DEFAULT_THREAD_STACK_SIZE; i += 4096)
        VirtualMemory::MapPage(rootTable, 0xffffa00000000000+i, stackAddr+i, PageFlags::present | PageFlags::readwrite | PageFlags::user);
    // Map the low 4GiBs at 0xffffffff80000000 or so
    for (size_t i = 0; i < 0x100000000; i += 4096)
        VirtualMemory::MapPage(rootTable, i+hhdm_req.response->offset, i, PageFlags::present | PageFlags::readwrite);
    
    next = nullptr;
}