#pragma once

#include <stdint.h>
#include <include/fs/vfs.h>

class Initrd : public Filesystem
{
public:
    Initrd(const char* mountpoint);

    virtual int open(const char* path, const char* permissions); // Permissions should be one of 'r', 'w', or 'rw' (more permissions to come)
    virtual ssize_t read(int fd, void* buf, size_t count);
    virtual int close(int fd);
    virtual off_t lseek(int fd, off_t offset, int whence);
};