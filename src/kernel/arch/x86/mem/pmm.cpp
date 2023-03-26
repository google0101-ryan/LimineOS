#include "pmm.h"
#include <kernel/drivers/screen.h>
#include <libk/string.h>
#include <kernel/arch/x86/util/spinlock.h>

static mutex_t pmm_mutex;

struct Bitmap
{
        uint8_t* buffer;
        bool operator[](uint64_t index);
        bool Set(uint64_t index, bool value);
        bool Get(uint64_t index);
};

bool Bitmap::operator[](uint64_t index)
{
        return Get(index);
}

bool Bitmap::Set(uint64_t index, bool value)
{
        uint64_t bytei = index / 8;
        uint8_t biti = index % 8;
        uint8_t bitindex = 0b10000000 >> biti;
        buffer[bytei] &= ~bitindex;
        if (value) buffer[bytei] |= bitindex;
        return true;
}

bool Bitmap::Get(uint64_t index)
{
        uint64_t bytei = index / 8;
    uint8_t biti = index % 8;
    uint8_t bitindexer = 0b10000000 >> biti;
    if ((buffer[bytei] & bitindexer) > 0) return true;
    return false;
}

static Bitmap bitmap;
uint64_t freeRam = 0;
uint64_t usedRam = 0;
uint64_t highest_address = 0;
uint64_t lastI = 0;

void PMM::Init(limine_memmap_response *mmap)
{
	mutex_init(&pmm_mutex);

	Terminal::SetScreenColors(0xFFFFFF, 0x000000);

	uint64_t bitmapSize;

	for (uint64_t i = 0; i < mmap->entry_count; i++)
	{
		printf("Found memory map entry from 0x%lx -> 0x%lx    ", mmap->entries[i]->base, mmap->entries[i]->base+mmap->entries[i]->length);
		if (mmap->entries[i]->type != LIMINE_MEMMAP_USABLE)
			printf("[USED]\n");
		else
			printf("[FREE]\n");
	}

	for (uint64_t i = 0; i < mmap->entry_count; i++)
	{
		if (mmap->entries[i]->type != LIMINE_MEMMAP_USABLE) continue;

		if (highest_address < (mmap->entries[i]->base + mmap->entries[i]->length))
			highest_address = (mmap->entries[i]->base + mmap->entries[i]->length);
		
		freeRam += mmap->entries[i]->length;
	}

	printf("%ldMiBs of free ram detected\n", (freeRam / 1024) / 1024);

	bitmapSize = ALIGN((highest_address / 0x1000) / 8, 0x1000);

	for (uint64_t i = 0; i < mmap->entry_count; i++)
	{
		if (mmap->entries[i]->type != LIMINE_MEMMAP_USABLE) continue;

		if (mmap->entries[i]->length >= bitmapSize)
		{
			bitmap.buffer = (uint8_t*)mmap->entries[i]->base;
			for (uint64_t i = 0; i < bitmapSize; i++)
				bitmap.buffer[i] = 0xFF;
			
			mmap->entries[i]->length -= bitmapSize;
			mmap->entries[i]->base += bitmapSize;
			freeRam -= bitmapSize;
			break;
		}
	}

	printf("Bitmap is at 0x%lx, %ld bytes long\n", bitmap.buffer, bitmapSize);

	for (size_t i = 0; i < mmap->entry_count; i++)
    {
        if (mmap->entries[i]->type != LIMINE_MEMMAP_USABLE) continue;

        for (uintptr_t t = 0; t < mmap->entries[i]->length; t += 0x1000)
        {
            bitmap.Set((mmap->entries[i]->base + t) / 0x1000, false);
        }
    }
}

void* InnerAlloc(uint64_t count, uint64_t limit)
{
	uint64_t p = 0;

	while (lastI < limit)
	{
		if (!bitmap.Get(lastI++))
		{
			if (++p == count)
			{
				uint64_t page = lastI - count;
				for (uint64_t i = page; i < lastI; i++) bitmap.Set(i, true);
				return reinterpret_cast<void*>(page * 0x1000);
			}
		}
		else p = 0;
	}

	return nullptr;
}

void *PMM::AllocPage(int count)
{
	mutex_lock(&pmm_mutex);

	uint64_t i = lastI;
	void* ret = InnerAlloc(count, highest_address / 0x1000);
	if (!ret)
	{
		lastI = 0;
		ret = InnerAlloc(count, i);
		if (ret == nullptr)
		{
			printf("Out of memory!\n");
			asm volatile("cli");
			for (;;)
				asm volatile("hlt");
		}
	}

	memset(ret, 0, count*0x1000);

	usedRam += count*0x1000;
	freeRam -= count*0x1000;

	mutex_unlock(&pmm_mutex);

	return ret;
}

void PMM::FreePage(void *ptr, size_t count)
{
	mutex_lock(&pmm_mutex);

	if (ptr == nullptr) return;

	size_t page = reinterpret_cast<size_t>(ptr) / 0x1000;
	for (size_t i = page; i < page+count; i++) bitmap.Set(i, false);
	if (lastI > page) lastI = page;

	usedRam -= count*0x1000;
	freeRam += count*0x1000;

	mutex_unlock(&pmm_mutex);
}
