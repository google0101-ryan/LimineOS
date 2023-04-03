#include "vfs.h"
#include <kernel/libk/string.h>
#include <kernel/drivers/screen.h>
#include <kernel/arch/x86/mem/vmm.h>
#include <kernel/arch/x86/mem/pmm.h>
#include <kernel/arch/x86/scheduler/scheduler.h>
#include <kernel/arch/x86/cpu.h>

#define VFS_GET_INFO() \
	Filesystem* mp = root_fs; \
	descriptors.get(fd, mp)

VFS* vfs;

void VFS::Init()
{
	vfs = new VFS();
}

#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_OSABI 7
#define EI_ABIVERSION 8

#define ELFCLASS64 2

#define ELFDATA2LSB 1

#define ELFOSABI_SYSV 0

struct Elf64_Ehdr
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

struct Elf64_Phdr
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

uint64_t ElfToFlags(uint64_t p_flags)
{
	uint64_t ret = VirtualMemory::flags::present;
	uint64_t x = p_flags & 1;
	uint64_t w = (p_flags >> 1) & 1;
	uint64_t r = (p_flags >> 2) & 1;
	
	if (!x)
		ret |= VirtualMemory::flags::executedisable;
	if (w)
		ret |= VirtualMemory::flags::writeable;
	
	return ret;
}

int VFS::LoadAndExec(const char *path)
{
	int fd = open(path);

	if (fd < 0)
		return -1;
	
	seek(fd, SEEK_END, 0);
	size_t size = tell(fd);
	seek(fd, SEEK_SET, 0);

	printf("Loading %s, %ld bytes\n", path, size);

	char* buf = new char[size];

	size_t bytesRead = read(fd, buf, size);

	if (bytesRead < size)
		return -2;
	
	if (*(uint32_t*)buf != 0x464c457f) // \x7fELF
	{
		printf("0x%08x\n", *(uint32_t*)buf);
		return -3;
	}

	Elf64_Ehdr* hdr = (Elf64_Ehdr*)buf;

	if (hdr->e_ident[EI_CLASS] != ELFCLASS64)
		return -4;
	
	if (hdr->e_ident[EI_DATA] != ELFDATA2LSB)
		return -5;
	
	if (hdr->e_ident[EI_OSABI] != ELFOSABI_SYSV)
		return -6;
	
	if (hdr->e_type != 2)
	{
		printf("Unhandled header type %d\n", hdr->e_type);
		return -7;
	}

	printf("File program headers start at 0x%x\n", hdr->e_phoff);

	auto thread = new Thread(hdr->e_entry, true);

	Elf64_Phdr* phdrs = (Elf64_Phdr*)(buf + hdr->e_phoff);

	for (int i = 0; i < hdr->e_phnum; i++)
	{
		if (phdrs[i].p_type != 1) continue;

		uint64_t vaddr = phdrs[i].p_vaddr;
		uint64_t paddr = (uint64_t)PMM::AllocPage(ALIGN(phdrs[i].p_memsz, phdrs[i].p_align) / 0x1000);

		VirtualMemory::MapPage(Scheduler::GetCurrentThread()->addressSpace, paddr, vaddr, VirtualMemory::flags::present | VirtualMemory::flags::writeable);
		VirtualMemory::MapPage(thread->addressSpace, paddr, vaddr, ElfToFlags(phdrs[i].p_flags) | VirtualMemory::flags::user);
		
		printf("Mapped ELF page 0x%08lx\n", vaddr);

		memcpy((void*)vaddr, buf + phdrs[i].p_offset, phdrs[i].p_filesz);

		if (phdrs[i].p_filesz < phdrs[i].p_memsz)
		{
			size_t zeroMem = phdrs[i].p_memsz - phdrs[i].p_filesz;
			memset((void*)(vaddr + phdrs[i].p_filesz), 0, zeroMem);
		}
	}

	Scheduler::RegisterThread(thread);

	return 0;
}

void VFS::mount(Filesystem *fs, const char *mountpoint, bool root)
{
	Mountpoint mp;
	mp.fs = fs;
	mp.name = mountpoint;
	mountpoints.push_back(mp);

	if (root)
		root_fs = fs;

	printf("Mounted FS of type %s on %s\n", fs->name, mountpoint);
}

int VFS::open(const char *path)
{
	Filesystem* mp = root_fs;
	const char* mp_path;

	for (size_t i = 0; i < mountpoints.size(); i++)
    {
        auto mount = mountpoints[i];
		if (!mount.name) continue;
        if (!strncmp(path, mount.name, strlen(mount.name)))
        {
            mp = mount.fs;
            mp_path = mount.name;
        }
    }

    const char* rel_path = path+strlen(mp_path);

	int fd = mp->open(rel_path);
	descriptors.put(fd, mp);

	return fd;
}

void VFS::close(int fd)
{
	VFS_GET_INFO();
	mp->close(fd);
	descriptors.remove(fd);
}

int VFS::write(int fd, char *buf, size_t size)
{
	VFS_GET_INFO();

	return mp->write(fd, buf, size);
}

int VFS::read(int fd, char* buf, size_t size)
{
	VFS_GET_INFO();

	return mp->read(fd, buf, size);
}

int VFS::seek(int fd, int seek_dir, size_t off)
{
	VFS_GET_INFO();

	return mp->seek(fd, seek_dir, off);
}

size_t VFS::tell(int fd)
{
	VFS_GET_INFO();

	return mp->tell(fd);
}
