#pragma once

inline static long syscall1(long num, long arg0)
{
    long result;
    asm volatile("syscall" : "=a"(result) : "a"(num), "b"(arg0));
    return result;
}