#pragma once

#include <stdint.h>

#define _NUM_SYSCALL_EXIT 0
#define _NUM_SYSCALL_LOG 1

static uint64_t syscall0(int sc)
{
    uint64_t ret;
    asm volatile("syscall" : "=a"(ret) : "a"(sc) : "rcx", "r11", "memory");
    return ret;
}

static uint64_t syscall1(int sc, uint64_t arg1)
{
    uint64_t ret;
    asm volatile("syscall" 
                    : "=a"(ret) 
                    : "a"(sc), "D"(arg1)
                    : "rcx", "r11", "memory");
    return ret;
}