#pragma once

#include <stdint.h>
#include <stddef.h>
#include <kernel/libk/list.h>
#include <kernel/libk/hashmap.h>

class Filesystem
{
protected:
	char name[128];
public:
	Filesystem();

	virtual int open(const char* name) = 0;
    virtual void close(int fd) = 0;
    virtual int write(int fd, char* buf, size_t size) = 0;
    virtual int read(int fd, char* buf, size_t size) = 0;
    virtual int seek(int fd, int seek_dir, size_t off) = 0;
    virtual size_t tell(int fd) = 0;
};

class VFS
{
private:
	Filesystem* root_fs; // Initially the initrd, once we get disk and FS drivers, it'll be the root FS of the boot disk

	struct Mountpoint
	{
		const char* name;
		Filesystem* fs;
	};

	List<Mountpoint> mountpoints;
	HashMap<int, Filesystem*> descriptors;

	VFS() {}
public:
	static void Init();

	VFS(VFS&) = delete;
    VFS& operator=(VFS&) = delete;
	
	void mount(Filesystem* fs, const char* mountpoint);

    int open(const char* path);
    void close(int fd);
    int write(int fd, char* buf, size_t size);
    int read(int fd, char* buf, size_t size);
    int seek(int fd, int seek_dir, size_t off);
    size_t tell(int fd);
};

static VFS* vfs;