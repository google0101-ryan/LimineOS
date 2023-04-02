#pragma once

#include <stdint.h>
#include <kernel/limine.h>
#include <kernel/arch/x86/util/spinlock.h>

namespace Terminal
{

void Init(limine_framebuffer_response* term);

void printf(const char* fmt, ...);

void SetScreenColors(uint32_t fg, uint32_t bg);

int GetCurX();
int GetCurY();

void SetXY(int x, int y);

void GetWidthHeight(int& w, int& h);

void ClearScreen();

}

#define printf Terminal::printf