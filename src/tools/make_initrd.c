#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc, char** argv)
{
    if (argc < 4 || argc % 2)
    {
        printf("Usage: %s <initrd out> <file list>\n", argv[0]);
        printf("\tWhere each file entry is a host path and a initrd path\n");
        return -1;
    }

    int num_files = argc - 2;
    num_files /= 2;

    int argc_pos = 2;

    FILE* out = fopen(argv[1], "wb");
    fwrite(&num_files, sizeof(int), 1, out);

    printf("%d files in initrd\n", num_files);

    size_t pos = sizeof(int) + (sizeof(struct initrd_file_header) * num_files);

    for (int i = 0; i < num_files; i++)
    {
        char* host_filename = argv[argc_pos];
        char* initrd_filename = argv[argc_pos+1];
        argc_pos += 2;

        printf("File %d: %s -> %s\n", i+1, host_filename, initrd_filename);

        FILE* f = fopen(host_filename, "rb");
        
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        struct initrd_file_header hdr;

        strncpy(hdr.name, initrd_filename, 128);
        hdr.pos = pos;
        hdr.size = size;

        fwrite(&hdr, sizeof(struct initrd_file_header), 1, out);

        fclose(f);
    }

    argc_pos = 2;

    for (int i = 0; i < num_files; i++)
    {
        char* host_filename = argv[argc_pos];
        argc_pos += 2;

        FILE* f = fopen(host_filename, "rb");
        
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        fseek(f, 0, SEEK_SET);

        char* buf = malloc(size);

        fread(buf, 1, size, f);

        fwrite(buf, 1, size, out);

        fclose(f);
    }

    fflush(out);
    fclose(out);

    return 0;
}