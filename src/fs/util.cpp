#include <include/fs/util.h>    
#include <include/fs/vfs.h>
#include <include/screen.h>
#include <include/mm/pmm.h>
#include <include/mm/vmm.h>
#include <include/sched/task.h>
#include <include/sched/scheduler.h>
#include <include/util.h>
#include <include/limine.h>

extern limine_hhdm_request hhdm_req;

struct Elf64Header
{
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct Elf64Phdr
{
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};

int FSUtils::LoadAndExecuteElf(const char *path)
{
    int fd = VFS::open(path, "rb");
    if (fd < 0)
    {
        return -1;
    }

    VFS::lseek(fd, 0, SEEK_END);
    size_t size = VFS::tell(fd);
    VFS::lseek(fd, 0, SEEK_SET);

    printf("0x%x\n", size);

    char* buf = new char[size];
    VFS::read(fd, (void*)buf, size);

    VFS::close(fd);

    Elf64Header* hdr = (Elf64Header*)buf;

    if (hdr->e_ident[0] != '\x7f'
        || hdr->e_ident[1] != 'E'
        || hdr->e_ident[2] != 'L'
        || hdr->e_ident[3] != 'F')
    {
        printf("ERROR: Tried to load invalid ELF file! %x%c%c%c\n",
                hdr->e_ident[0], hdr->e_ident[1], hdr->e_ident[2], hdr->e_ident[3]);
        return -1;
    }

    Elf64Phdr* phdrs = (Elf64Phdr*)(buf+hdr->e_phoff);

    Thread* t = new Thread(hdr->e_entry, true);

    for (int i = 0; i < hdr->e_phnum; i++)
    {
        printf("Found ELF phdr of type %d, vaddr 0x%x, filesz 0x%x, memsz 0x%x\n", phdrs[i].p_type, phdrs[i].p_vaddr, phdrs[i].p_filesz, phdrs[i].p_memsz);

        if (phdrs[i].p_type != 1 || !phdrs[i].p_memsz)
            continue;
        void* segment = PhysicalMemory::alloc((phdrs[i].p_memsz / 4096) < 1 ? 1 : (phdrs[i].p_memsz / 4096));
        t->MapPage(segment, phdrs[i].p_vaddr, PageFlags::present | PageFlags::readwrite | PageFlags::user);
        
        memcpy(segment+hhdm_req.response->offset, buf+phdrs[i].p_offset, phdrs[i].p_filesz);
    }

    Scheduler::AddThread(t);

    return 0;
}