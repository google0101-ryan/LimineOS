#include "vfs.h"
#include <kernel/libk/string.h>
#include <kernel/drivers/screen.h>

#define VFS_GET_INFO() \
	Filesystem* mp = root_fs; \
	descriptors.get(fd, mp)

void VFS::Init()
{
	vfs = new VFS();
}

void VFS::mount(Filesystem *fs, const char *mountpoint)
{
	mountpoints.push_back({mountpoint, fs});

	printf("Mounted FS of type %s on %s\n", fs->name, mountpoint);
}

int VFS::open(const char *path)
{
	Filesystem* mp = root_fs;
	const char* mp_path;

	for (size_t i = 0; i < mountpoints.size(); i++)
    {
        auto mount = mountpoints[i];
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
