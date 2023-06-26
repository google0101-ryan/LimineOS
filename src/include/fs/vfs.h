#pragma once

#include <stdint.h>
#include <stddef.h>

typedef long int off_t;
typedef long ssize_t;

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

class Filesystem
{
private:
    const char* name; // ISO9660, FAT32, EXT2, etc
    const char* mp; // Where this particular FS is mounted, with '/' being the root fs (example, devfs will be have mp = '/dev')
public:
    Filesystem(const char* mp, const char* name)
    : mp(mp), name(name) {}

    virtual int open(const char* path, const char* permissions) = 0; // Permissions should be one of 'r', 'w', or 'rw' (more permissions to come)
    virtual ssize_t read(int fd, void* buf, size_t count) = 0;
    virtual int close(int fd) = 0;
    virtual off_t lseek(int fd, off_t offset, int whence) = 0;

    const char* GetMountpoint() {return mp;}
};

namespace VFS
{

void Initialize();

void mount(Filesystem* fs);

}