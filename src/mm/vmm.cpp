#include <include/mm/pmm.h>
#include <include/mm/vmm.h>
#include <include/screen.h>
#include <include/limine.h>
#include <include/util.h>

limine_hhdm_request hhdm_req = 
{
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,
    .response = nullptr
};

limine_kernel_address_request kernel_addr = 
{
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0,
    .response = nullptr
};

limine_kernel_file_request kernel_file =
{
    .id = LIMINE_KERNEL_FILE_REQUEST,
    .revision = 0,
    .response = nullptr
};

extern limine_memmap_request mmap_req;

PML4Table* VirtualMemory::kernelDir;

void VirtualMemory::MapPage(PML4Table* root, uint64_t vaddr, uint64_t paddr, uint64_t flags)
{
    root = (PML4Table*)((uint8_t*)root + hhdm_req.response->offset);

    uint64_t pml4index = (vaddr >> 39) & 0x1ff;
    uint64_t dirptrindex = (vaddr >> 30) & 0x1ff;
    uint64_t dirindex = (vaddr >> 21) & 0x1ff;
    uint64_t tableindex = (vaddr >> 12) & 0x1ff;

    if (!root->pdpte[pml4index])
    {
        uint64_t pml4e_addr = (uint64_t)PhysicalMemory::alloc(1);
        memset((void*)(pml4e_addr+hhdm_req.response->offset), 0, sizeof(PageDirPtrTable));
        pml4e_addr |= PageFlags::present | PageFlags::readwrite | PageFlags::user; // We set default permissions, so the permissions are page granularity
        root->pdpte[pml4index] = pml4e_addr;
    }

    PageDirPtrTable* pdpt = (PageDirPtrTable*)(root->pdpte[pml4index] & ~FLAG_MASK);
    pdpt = (PageDirPtrTable*)((uint8_t*)pdpt + hhdm_req.response->offset);

    if (!pdpt->pdpte[dirptrindex])
    {
        uint64_t pdpte_addr = (uint64_t)PhysicalMemory::alloc(1);
        memset((void*)(pdpte_addr+hhdm_req.response->offset), 0, sizeof(PageDirPtrTable));
        pdpte_addr |= PageFlags::present | PageFlags::readwrite | PageFlags::user; // We set default permissions, so the permissions are page granularity
        pdpt->pdpte[dirptrindex] = pdpte_addr;
    }

    PageDirectory* pdt = (PageDirectory*)(pdpt->pdpte[dirptrindex] & ~FLAG_MASK);
    pdt = (PageDirectory*)((uint8_t*)pdt + hhdm_req.response->offset);

    if (!pdt->pde[dirindex])
    {
        uint64_t pdte_addr = (uint64_t)PhysicalMemory::alloc(1);
        memset((void*)(pdte_addr+hhdm_req.response->offset), 0, sizeof(PageDirPtrTable));
        pdte_addr |= PageFlags::present | PageFlags::readwrite | PageFlags::user; // We set default permissions, so the permissions are page granularity
        pdt->pde[dirindex] = pdte_addr;
    }

    PageTable* pt = (PageTable*)(pdt->pde[dirindex] & ~FLAG_MASK);
    pt = (PageTable*)((uint8_t*)pt + hhdm_req.response->offset);
    pt->pte[tableindex] = paddr | flags;
}

void VirtualMemory::Initialize()
{
    kernelDir = (PML4Table*)PhysicalMemory::alloc(sizeof(PML4Table)/4096);
    memset(kernelDir, 0, sizeof(PML4Table));

    printf("Kernel page directory is at 0x%x\n", kernelDir);

    printf("Kernel is 0x%x -> 0x%x, vaddr 0x%x -> 0x%x\n", kernel_addr.response->physical_base, kernel_addr.response->physical_base+kernel_file.response->kernel_file->size, kernel_addr.response->virtual_base, kernel_addr.response->virtual_base+kernel_file.response->kernel_file->size);

    // Map the kernel
    for (size_t i = 0; i < kernel_file.response->kernel_file->size; i += 4096)
        MapPage(kernelDir, kernel_addr.response->virtual_base+i, kernel_addr.response->physical_base+i, PageFlags::present | PageFlags::readwrite);
    
    // Map the low 4GiBs at 0xffffffff80000000 or so
    for (size_t i = 0; i < 0x100000000; i += 4096)
        MapPage(kernelDir, i+hhdm_req.response->offset, i, PageFlags::present | PageFlags::readwrite);

    auto mmap = mmap_req.response->entries;
    uint64_t phyAddr = 0;
    uint64_t len = 0;

    // Map the framebuffer
    for (size_t i = 0; i < mmap_req.response->entry_count; i++)
    {
        if (mmap[i]->type != LIMINE_MEMMAP_FRAMEBUFFER) continue;

        phyAddr = mmap[i]->base;
        len = mmap[i]->length;
        break;
    }

    if (phyAddr == 0)
    {
        printf("Error getting framebuffer address!\n");
        Utils::HaltCatchFire();
    }

    printf("Found framebuffer at physaddr 0x%x\n", phyAddr);

    for (size_t i = 0; i < len; i += 4096)
        MapPage(kernelDir, reinterpret_cast<uint64_t>(Screen::GetFramebuffer())+i, phyAddr+i, PageFlags::present | PageFlags::readwrite);

    SwapToPageTable(kernelDir);
}

void VirtualMemory::SwapToKernelPT()
{
    SwapToPageTable(kernelDir);
}

void VirtualMemory::SwapToPageTable(PML4Table * page)
{
    asm volatile("mov %0, %%cr3" ::"r"(page));
}