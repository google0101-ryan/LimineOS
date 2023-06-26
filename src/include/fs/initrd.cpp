#include <include/fs/initrd.h>
#include <lib/hash.h>
#include <lib/doubly.h>
#include <include/util.h>
#include <include/limine.h>
#include <include/screen.h>

static int cur_fd = 0;

// Removes sub from str, and returns the result
// Modifies str
const char* strremove(const char* str, const char* sub)
{
    size_t len = strlen(sub);
    if (len > 0)
    {
        char* p = (char*)str;
        while ((p = (char*)strstr(p, sub)) != nullptr)
            memmove(p, p + len, strlen(p + len) + 1);
    }
    return str;
}

struct FileInfo
{
    uint8_t* data;
    uint32_t size;
    char name[64];
    size_t cur_pos;
};

struct [[gnu::packed]] FileHeader
{
    uint32_t offs;
    uint32_t size;
    char name[64];
};

std::unordered_map<int, FileInfo*> open_files;
std::vector<FileInfo> files;

volatile limine_module_request mods = 
{
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0,
    .response = nullptr,
    .internal_module_count = 0,
    .internal_modules = nullptr
};

Initrd::Initrd(const char *mountpoint)
: Filesystem(mountpoint, "initial ramdisk")
{
    if (mods.response->module_count <= 0)
    {
        printf("ERROR: No modules detected! Unable to mount initrd on \"%s\"\n", mountpoint);
        Utils::HaltCatchFire();
    }

    limine_file* initrd_module;

    for (size_t i = 0; i < mods.response->module_count; i++)
    {
        auto& mod = mods.response->modules[i];

        if (!strncmp(mod->cmdline, "initrd.img", 10))
        {
            printf("Found initrd\n");
            initrd_module = mod;
        }
    }

    if (!initrd_module)
    {
        printf("ERROR: No modules detected! Unable to mount initrd on \"%s\"\n", mountpoint);
        Utils::HaltCatchFire();
    }

    uint32_t fileCount = *(uint32_t*)initrd_module->address;

    printf("%d files in initrd\n", fileCount);

    for (size_t i = 0; i < fileCount; i++)
    {
        FileHeader* hdr = (FileHeader*)(initrd_module->address+4+(i*sizeof(FileHeader)));
        FileInfo info;
        info.cur_pos = 0;
        info.data = (uint8_t*)(initrd_module->address+hdr->offs);
        memcpy(info.name, ((char*)hdr)+8, 64);
        info.size = hdr->size;
        files.push_back(info);
        hdr++;

        char str[64] = {0};
        memcpy(str, info.data, info.size);
        printf("%s\n", str);
    }

    int fd = open("/hello.txt", "rb");

    if (fd < 0)
    {
        printf("ERROR: unable to open /hello.text\n");
        Utils::HaltCatchFire();
    }
}

int Initrd::open(const char *path, const char *permissions)
{
    path = strremove(path, GetMountpoint());
    FileInfo* info = nullptr;

    printf("Relative path: %s\n", path);

    for (size_t i = 0; i < files.size(); i++)
    {
        if (!strncmp(files[i].name, path, 128))
        {
            info = &files[i];
            break;
        }
    }

    if (!info)
        return -1;
    
    info->cur_pos = 0;
    open_files.put(cur_fd, info);
    return cur_fd++;
}

ssize_t Initrd::read(int fd, void *buf, size_t count)
{
    FileInfo* info;
    if (!open_files.get(fd, info))
        return -1;
    
    if (count+info->cur_pos > info->size)
        return -1;
    
    if (info->data == nullptr) // This should never happen, but just in case...
        return -1;
    
    memcpy(buf, info->data+info->cur_pos, count);
    info->cur_pos += count;

    return count;
}

int Initrd::close(int fd)
{
    cur_fd--;
    open_files.remove(fd);
}

off_t Initrd::lseek(int fd, off_t offset, int whence)
{
    FileInfo* info;
    if (!open_files.get(fd, info))
        return -1;

    size_t pos;
    switch (whence)
    {
    case SEEK_SET:
        info->cur_pos = pos;
        break;
    case SEEK_CUR:
        info->cur_pos += pos;
        break;
    case SEEK_END:
        info->cur_pos = info->size + pos;
        break;
    }

    return info->cur_pos;
}
