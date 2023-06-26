#include <fstream>
#include <string.h>

struct FileHeader
{
    uint32_t offs;
    uint32_t size;
    char name[64];
};

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        printf("Usage: %s <output> <files...>\n", argv[0]);
        exit(1);
    }

    std::ofstream out(argv[1]);
    int count = argc - 2;
    out.write((char*)&count, 4);

    size_t offs = (sizeof(FileHeader) * count) + 4;

    for (int i = 0; i < count; i++)
    {
        FileHeader hdr;

        std::string name(argv[i+2]);
        name = name.substr(name.find_last_of("/\\") + 1);

        strncpy(hdr.name, name.c_str(), 64);

        printf("Adding \"%s\"\n", name.c_str());

        std::ifstream in(argv[i+2], std::ios::ate);
        size_t size = in.tellg();
        in.seekg(0, std::ios::end);
        hdr.size = size;
        hdr.offs = offs;

        out.write((char*)&hdr, sizeof(FileHeader));

        in.close();
    }

    // Write the actual file data
    offs = (sizeof(FileHeader) * count) + 4;
    out.seekp(offs, std::ios::beg);
    for (int i = 0; i < count; i++)
    {
        std::ifstream in(argv[i+2], std::ios::ate);
        size_t size = in.tellg();
        in.seekg(0, std::ios::beg);
        char* buf = new char[size];
        in.read(buf, size);

        out.write(buf, size);

        delete buf;
        in.close();
    }

    out.close();
    return 0;
}