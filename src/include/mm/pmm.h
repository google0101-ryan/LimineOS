#pragma once

#include <stdint.h>
#include <stddef.h>

#define DIV_ROUNDUP(A, B) \
({ \
    __typeof__(A) _a_ = A; \
    __typeof__(B) _b_ = B; \
    (_a_ + (_b_ - 1)) / _b_; \
})


#define ALIGN_UP(A, B) \
({ \
    __typeof__(A) _a__ = A; \
    __typeof__(B) _b__ = B; \
    DIV_ROUNDUP(_a__, _b__) * _b__; \
})

#define ALIGN_DOWN(A, B) \
({ \
    __typeof__(A) _a_ = A; \
    __typeof__(B) _b_ = B; \
    (_a_ / _b_) * _b_; \
})

namespace PhysicalMemory
{

void Initialize();

void* alloc(size_t size);
void free(void* ptr, size_t size);

}