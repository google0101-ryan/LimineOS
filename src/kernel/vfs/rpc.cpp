#include "rpc.h"
#include <kernel/libk/string.h>

RpcFilesystem::RpcFilesystem()
{
    strncpyz(name, "RPC", 3);
}

int RpcFilesystem::open(const char *name)
{
    return 0;
}

void RpcFilesystem::close(int fd)
{
}

int RpcFilesystem::read(int fd, char *buf, size_t size)
{
    return 0;
}

int RpcFilesystem::seek(int fd, int seek_dir, size_t off)
{
    return 0;
}

size_t RpcFilesystem::tell(int fd)
{
    return size_t();
}
