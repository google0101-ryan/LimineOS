#pragma once

int _printf(const char* fmt, ...);

#define printf _printf

namespace Screen
{

void Initialize();

volatile void* GetFramebuffer();

}