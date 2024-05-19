#include <include/fs/vfs.h>
#include <lib/doubly.h>
#include <lib/hash.h>
#include <include/fs/initrd.h>
#include <include/screen.h>
#include <include/util.h>

std::vector<Filesystem*> filesystems;
std::unordered_map<int, Filesystem*> openFilesystems;

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

int VFS::open(const char *path, const char *mode)
{
    Filesystem* fs = NULL;
    for (int i = 0; i < filesystems.size(); i++)
    {
        auto filesystem = filesystems[i];

        printf("%d\n", strlen(filesystem->GetMountpoint()));

        if (!strncmp(filesystem->GetMountpoint(), path, strlen(filesystem->GetMountpoint())))
        {
            fs = filesystem;  
        }
    }

    if (!fs)
    {
        printf("ERROR: Couldn't open '%s': Invalid mountpoint\n", path);
        return -1;
    }

    path += strlen(fs->GetMountpoint());

    int fd = fs->open(path, mode);
    openFilesystems.put(fd, fs);
    return fd;
}

ssize_t VFS::read(int fd, void *buf, size_t count)
{
    Filesystem* fs;
    if (!openFilesystems.get(fd, fs))
        return -1;
    
    return fs->read(fd, buf, count);
}

int VFS::close(int fd)
{
    Filesystem* fs;
    if (!openFilesystems.get(fd, fs))
        return -1;
    
    return fs->close(fd);
}

off_t VFS::lseek(int fd, off_t offset, int whence)
{
    Filesystem* fs;
    if (!openFilesystems.get(fd, fs))
        return -1;
    
    return fs->lseek(fd, offset, whence);
}

off_t VFS::tell(int fd)
{
    Filesystem* fs;
    if (!openFilesystems.get(fd, fs))
        return -1;
    
    return fs->tell(fd);
}
