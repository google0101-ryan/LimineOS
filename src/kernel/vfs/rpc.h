#pragma once

#include <stdint.h>
#include <kernel/vfs/vfs.h>

// This will be mounted on /rpc
// Each file will represent a two-way FIFO owned by a process
class RpcFilesystem : public Filesystem
{
private:
public:
    RpcFilesystem();

    virtual int open(const char* name);
    virtual void close(int fd);
    virtual int write(int, char*, size_t) {return -1;}
    virtual int read(int fd, char* buf, size_t size);
    virtual int seek(int fd, int seek_dir, size_t off);
    virtual size_t tell(int fd);
};