#include <include/fs/vfs.h>
#include <lib/doubly.h>
#include <include/fs/initrd.h>

std::vector<Filesystem*> filesystems;

void VFS::Initialize()
{
    Initrd* initrd = new Initrd("/"); // Mount our initrd as the (temporary) root

    VFS::mount(initrd);
}

void VFS::mount(Filesystem *fs)
{
    filesystems.push_back(fs);
}