#pragma once

#include <stdint.h>
#include <stddef.h>

namespace HPET
{

void SetupHPET();

}

void msleep(size_t ms);