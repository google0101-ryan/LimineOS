#include <stdint.h>
#include <syscall.h>

int main(int argc, char** argv)
{
    const char* msg = "Hello, world!\n";
    syscall1(_NUM_SYSCALL_LOG, (uint64_t)msg);
    return 0;
}