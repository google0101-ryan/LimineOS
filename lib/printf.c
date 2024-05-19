#include "syscall.h"

#ifdef __cplusplus
extern "C" {
#endif

void putc(char c)
{
    syscall1(0x1, c);
}

void puts(const char* str)
{
    while (*str)
        putc(*str++);
}

#ifdef __cplusplus
}
#endif