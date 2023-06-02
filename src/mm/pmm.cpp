#include <include/mm/pmm.h>
#include <include/limine.h>
#include <include/screen.h>
#include <include/util.h>

limine_memmap_request mmap_req = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
    .response = nullptr,
};

extern limine_hhdm_request hhdm_req;

static uintptr_t highest_addr = 0;
static uint64_t lastI = 0;
static uint64_t usedRam = 0;
static uint64_t freeRam = 0;

struct Bitmap
{
    uint8_t* buffer = nullptr;
    bool operator[](size_t index)
    {
        return Get(index);
    }

    bool Set(uint64_t index, bool value)
    {
        uint64_t bytei = index / 8;
        uint8_t biti = index % 8;
        uint8_t bitindexer = 0b10000000 >> biti;
        buffer[bytei] &= ~bitindexer;
        if (value) buffer[bytei] |= bitindexer;
        return true;
    }

    bool Get(uint64_t index)
    {
        uint64_t bytei = index / 8;
        uint8_t biti = index % 8;
        uint8_t bitindexer = 0b10000000 >> biti;
        if ((buffer[bytei] & bitindexer) != 0) return true;
        return false;
    }
} bitmap;

void PhysicalMemory::Initialize()
{
    printf("%d entries present in memory map\n", mmap_req.response->entry_count);

    auto mmap = mmap_req.response->entries;

    for (uint64_t i = 0; i < mmap_req.response->entry_count; i++)
    {
        if (mmap[i]->type == LIMINE_MEMMAP_USABLE)
        {
            uintptr_t top = mmap[i]->base + mmap[i]->length;
            freeRam += mmap[i]->length;

            if (top > highest_addr) highest_addr = top;
        }
    }

    uint64_t bitmapSize = ALIGN_UP((highest_addr / 4096) / 8, 4096);

    for (uint64_t i = 0; i < mmap_req.response->entry_count; i++)
    {
        if (mmap[i]->type != LIMINE_MEMMAP_USABLE) continue;

        if (mmap[i]->length >= bitmapSize)
        {
            bitmap.buffer = (uint8_t*)(mmap[i]->base + hhdm_req.response->offset);
            memset(bitmap.buffer, 0xFF, bitmapSize);

            mmap[i]->length -= bitmapSize;
            mmap[i]->base += bitmapSize;
            freeRam -= bitmapSize;
            break;
        }
    }

    for (size_t i = 0; i < mmap_req.response->entry_count; i++)
    {
        if (mmap[i]->type != LIMINE_MEMMAP_USABLE) continue;

        for (uintptr_t t = 0; t < mmap[i]->length; t += 4096)
        {
            bitmap.Set((mmap[i]->base + t) / 4096, false);
        }
    }

    printf("%dMiBs of usable memory found (bitmap is %d bytes, at 0x%x)\n", freeRam/1024/1024, bitmapSize, bitmap.buffer);
}

static void* inner_alloc(size_t count, size_t limit)
{
    size_t p = 0;

    while (lastI < limit)
    {
        if (!bitmap[lastI++])
        {
            if (++p == count)
            {
                size_t page = lastI - count;
                for (size_t i = page; i < lastI; i++) bitmap.Set(i, true);
                return reinterpret_cast<void*>(page * 4096);
            }
        }
        else
            p = 0;
    }
    return nullptr;
}

void *PhysicalMemory::alloc(size_t size)
{
    size_t i = lastI;
    void* ret = inner_alloc(size, highest_addr / 0x1000);
    if (ret == nullptr)
    {
        lastI = 0;
        ret = inner_alloc(size, i);
        if (ret == nullptr)
        {
            printf("ERROR: Out of memory\n");
            Utils::HaltCatchFire();
        }
        memset(reinterpret_cast<void*>(reinterpret_cast<uint64_t>(ret) + hhdm_req.response->offset), 0, size*4096);

        usedRam += size*0x1000;
        freeRam -= size*0x1000;
    }

    return ret;
}

void PhysicalMemory::free(void *ptr, size_t size)
{
    size_t page = (size_t)ptr / 0x1000;
    for (size_t i = page; i < page + size; i++) bitmap.Set(i, 0);
    if (lastI > page) lastI = page;

    usedRam -= size*0x1000;
    freeRam += size*0x1000;
}