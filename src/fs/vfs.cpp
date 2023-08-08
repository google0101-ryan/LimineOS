#include <include/fs/vfs.h>
#include <lib/doubly.h>
#include <include/fs/initrd.h>
#include <include/screen.h>

std::vector<Filesystem*> filesystems;

void VFS::Initialize()
{
    Initrd* initrd = new Initrd("/"); // Mount our initrd as the (temporary) root

    VFS::mount(initrd);
}

void VFS::mount(Filesystem *fs)
{
    printf("Mounted fs of type \"%s\" on \"%s\"\n", fs->GetName(), fs->GetMountpoint());
    filesystems.push_back(fs);
}