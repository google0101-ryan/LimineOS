#pragma once

#include <stddef.h>

void memset(void* a, int c, size_t size);

int strncmp(const char* a, const char* b, size_t size);
size_t strlen(const char* a);
void strncpyz(char* dst, const char* src, size_t size);