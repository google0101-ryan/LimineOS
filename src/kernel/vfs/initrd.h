#pragma once

#include <kernel/vfs/vfs.h>

class Initrd : public Filesystem
{
private:
    // We support a max of 256 open files
    bool fds[256];

    struct initrd_header
    {
        int num_files;
    };

    struct initrd_file_header
    {
        char name[128];
        size_t size;
        size_t pos;
    };

    struct FileDescriptor
    {
        int fd;
        size_t pos;
        initrd_file_header* hdr;
    };

    void* startOfFiles;

    initrd_file_header* headers;
    initrd_header* hdr;

    HashMap<int, FileDescriptor> open_files;
public:
    Initrd();

    virtual int open(const char* name);
    virtual void close(int fd);
    virtual int write(int, char*, size_t) {return -1;}
    virtual int read(int fd, char* buf, size_t size);
    virtual int seek(int fd, int seek_dir, size_t off);
    virtual size_t tell(int fd);
};