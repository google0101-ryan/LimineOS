#include "initrd.h"
#include <kernel/limine.h>
#include <kernel/libk/string.h>
#include <kernel/drivers/screen.h>
#include <kernel/arch/x86/arch.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static volatile limine_module_request mod_req =
{
	.id = LIMINE_MODULE_REQUEST,
	.response = nullptr
};

#define INITRD_GET_INFO() FileDescriptor fileDescriptor; \
						  if (!open_files.get(fd, fileDescriptor)) \
						  	return -1;

Initrd::Initrd()
{
	strncpyz(name, "initrd", 6);

	printf("Initializing initrd\n");
	limine_file* initrd;

	printf("Searching through %ld modules\n", mod_req.response->module_count);

	for (size_t i = 0; i < mod_req.response->module_count; i++)
	{
		auto mod = mod_req.response->modules[i];
		
		if (!strncmp(mod->cmdline, "initrd.img", 10))
		{
			initrd = mod;
			break;
		}
	}

	if (!initrd)
	{
		printf("ERROR: Initrd not found!\n");
		Arch::Halt();
	}

	hdr = (initrd_header*)initrd->address;

	printf("Initrd is at 0x%lx, contains %d files\n", (uint64_t)hdr, hdr->num_files);

	headers = (initrd_file_header*)((char*)hdr + sizeof(initrd_header));

	printf("/\n");

	for (size_t i = 0; i < hdr->num_files; i++)
	{
		printf("|--- %s\n", headers[i].name);
	}

	startOfFiles = ((char*)hdr + sizeof(initrd_header)) + (sizeof(initrd_file_header) * hdr->num_files);
}

int Initrd::open(const char *name)
{
	if (*name == '\0')
		return -1;
	
	int file_num = -1;
	
	for (int i = 0; i < hdr->num_files; i++)
	{
		if (!strncmp(name, headers[i].name, 128))
		{
			file_num = i;
			break;
		}
	}

	if (file_num == -1)
		return -1;
	
	int fd = -1;
	
	for (int i = 0; i < 256; i++)
	{
		if (!fds[i])
		{
			fd = i;
			break;
		}
	}

	if (fd == -1)
		return -1;
	
	FileDescriptor new_file;
	new_file.fd = fd;
	new_file.hdr = &headers[file_num];
	new_file.pos = 0;

	open_files.put(fd, new_file);

	return fd;
}

void Initrd::close(int fd)
{
	open_files.remove(fd);
	fds[fd] = false;
}

int Initrd::read(int fd, char *buf, size_t size)
{
	if (!buf)
		return -1;
	
	INITRD_GET_INFO();

	char* pos = (char*)startOfFiles + fileDescriptor.hdr->pos + fileDescriptor.pos;

	size_t realSize = MIN(size, fileDescriptor.hdr->size);

	for (size_t i = 0; i < realSize; i++)
		buf[i] = pos[i];

	return realSize;
}

int Initrd::seek(int fd, int seek_dir, size_t off)
{
	INITRD_GET_INFO();

	if (off > fileDescriptor.hdr->size)
		off = fileDescriptor.hdr->size;

	switch (seek_dir)
	{
	case SEEK_SET:
		fileDescriptor.pos = off;
		break;
	case SEEK_CUR:
		fileDescriptor.pos += off;
		break;
	case SEEK_END:
		fileDescriptor.pos = fileDescriptor.hdr->size - off;
		break;
	}

	return 0;
}

size_t Initrd::tell(int fd)
{
	INITRD_GET_INFO();

	return fileDescriptor.pos;
}
