#pragma once

#include <stdint.h>
#include <stddef.h>
#include <include/screen.h>

static inline void *
memcpy (void *dest, const void *src, size_t len)
{
  char *d = (char*)dest;
  const char *s = (const char*)src;
  while (len--)
    *d++ = *s++;
  return dest;
}

static inline void *
memset (void *dest, int val, size_t len)
{
  unsigned char *ptr = (unsigned char*)dest;
  while (len-- > 0)
    *ptr++ = val;
  return dest;
}

static inline int
strncmp(const char *s1, const char *s2, size_t n)
{
  unsigned char u1, u2;

  while (n-- > 0)
    {
      u1 = (unsigned char) *s1++;
      u2 = (unsigned char) *s2++;
      if (u1 != u2)
	return u1 - u2;
      if (u1 == '\0')
	return 0;
    }
  return 0;
}

static inline int
strcmp(const char *s1, const char *s2)
{
    unsigned char u1, u2;

    while (1)
    {
        u1 = (unsigned char) *s1++;
        u2 = (unsigned char) *s2++;
        if (u1 != u2)
	        return u1 - u2;
        if (u1 == '\0')
	        return 0;
    }
    return 0;
}

static inline size_t strlen(const char* str)
{
    size_t i;
    while (*str++)
        i++;
    return i;
}

// returns true if `X` and `Y` are the same
static inline int compare(const char *X, const char *Y)
{
    while (*X && *Y)
    {
        if (*X != *Y) {
            return 0;
        }
 
        X++;
        Y++;
    }
 
    return (*Y == '\0');
}
 
// Function to implement `strstr()` function
static inline const char* strstr(const char* X, const char* Y)
{
    while (*X != '\0')
    {
        if ((*X == *Y) && compare(X, Y)) {
            return X;
        }
        X++;
    }
 
    return NULL;
}

static inline void* memmove(void* dest, const void* src, unsigned int n)
{
    unsigned char* pd = (unsigned char*)dest;
    const unsigned char* ps = (const unsigned char*)src;
    if (ps < pd)
        for (pd += n; ps += n; n--)
            *--pd = *--ps;
    else
        while (n--)
            *pd++ = *ps++;
    return dest;
}

namespace Utils
{

inline void DisableInts()
{
    asm volatile("cli");
}

inline void EnableInts()
{
    asm volatile("sti");
}

inline void HaltCatchFire [[gnu::noreturn]]()
{
    DisableInts();
    for (;;)
        asm volatile("hlt");
}

inline void Halt()
{
    for (;;)
        asm volatile("hlt");
}

inline void outb(uint16_t port, uint8_t data)
{
    asm volatile("outb %0, %1" :: "a"(data), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port)
                   : "memory");
    return ret;
}

inline void outw(uint16_t port, uint16_t data)
{
    asm volatile("outw %0, %1" :: "a"(data), "Nd"(port) : "memory");
}

static inline uint16_t inw(uint16_t port)
{
    uint16_t ret;
    asm volatile ( "inw %1, %0"
                   : "=a"(ret)
                   : "Nd"(port)
                   : "memory");
    return ret;
}

inline void outd(uint16_t port, uint32_t data)
{
    asm volatile("outl %0, %1" :: "a"(data), "Nd"(port) : "memory");
}

static inline uint32_t ind(uint16_t port)
{
    uint32_t ret;
    asm volatile ( "inl %1, %0"
                   : "=a"(ret)
                   : "Nd"(port)
                   : "memory");
    return ret;
}

static inline uint32_t ReadCr0()
{
    uint32_t ret;
    asm volatile("mov %%cr0, %%rax" : "=a"(ret));
    return ret;
}

static inline void WriteCr0(uint32_t data)
{
    asm volatile("mov %%rax, %%cr0" :: "a"(data));
}

static inline uint32_t ReadCr2()
{
    uint32_t ret;
    asm volatile("mov %%cr2, %%rax" : "=a"(ret));
    return ret;
}

static inline uint32_t ReadCr4()
{
    uint32_t ret;
    asm volatile("mov %%cr4, %%rax" : "=a"(ret));
    return ret;
}

static inline void WriteCr4(uint32_t data)
{
    asm volatile("mov %%rax, %%cr4" :: "a"(data));
}

static inline void wrmsr(uint64_t msr, uint64_t value)
{
	uint32_t low = value & 0xFFFFFFFF;
	uint32_t high = value >> 32;
	asm volatile (
		"wrmsr"
		:
		: "c"(msr), "a"(low), "d"(high)
	);
}

static inline uint64_t rdmsr(uint64_t msr)
{
	uint32_t low, high;
	asm volatile (
		"rdmsr"
		: "=a"(low), "=d"(high)
		: "c"(msr)
	);
	return ((uint64_t)high << 32) | low;
}

}