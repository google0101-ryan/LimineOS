#pragma once

#include <stdint.h>

class PATA
{
private:
public:
    PATA();

    unsigned int Read(uint64_t block, unsigned int nblocks, void* buffer);
};