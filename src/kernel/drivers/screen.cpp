#include "screen.h"

#define SSFN_CONSOLEBITMAP_TRUECOLOR
#define SSFN_CONSOLEBITMAP_CONTROL
#define __THROW
#include "ssfn.h"
#include "printf.h"

extern unsigned char _binary_unifont_sfn_start;

limine_framebuffer** framebuffers;
limine_framebuffer* main_fb;
uint64_t fb_count;

uint32_t cursorbuf[16*19];
uint32_t cursorbuf2[16*19];

uint32_t fgcolor = 0xFFFFFF;
uint32_t bgcolor = 0;

static mutex_t screen_mutex;

struct
{
	uint64_t x;
	uint64_t y;
} pos = {0, 0};

bool mouseDrawn;

void putc(char c, void*)
{
	ssfn_putc(c);
}

void setpos(uint64_t x, uint64_t y)
{
    pos.x = ssfn_dst.x = x * 8;
    pos.x = ssfn_dst.y = y * 16;
}

void resetpos()
{
    pos.x = ssfn_dst.x = 0;
    pos.x = ssfn_dst.y = 0;
}

#undef printf

void Terminal::printf(const char* fmt, ...)
{
	mutex_lock(&screen_mutex);

	va_list args;
	va_start(args, fmt);
	vfctprintf(&putc, nullptr, fmt, args);
	va_end(args);

	mutex_unlock(&screen_mutex);
}

void Terminal::SetScreenColors(uint32_t fg, uint32_t bg)
{
	fgcolor = fg;
	bgcolor = bg;
	ssfn_dst.bg = bg;
	ssfn_dst.fg = fg;
}

int Terminal::GetCurX()
{
	return ssfn_dst.x;
}

int Terminal::GetCurY()
{
	return ssfn_dst.y;
}

void Terminal::SetXY(int x, int y)
{
	ssfn_dst.y = y;
	ssfn_dst.x = x;
}

void Terminal::GetWidthHeight(int &w, int &h)
{
	w = ssfn_dst.w;
	h = ssfn_dst.h;
}

void Terminal::Init(limine_framebuffer_response *term)
{
	mutex_init(&screen_mutex);

	framebuffers = term->framebuffers;
	main_fb = term->framebuffers[0];
	fb_count = term->framebuffer_count;

	ssfn_src = reinterpret_cast<ssfn_font_t*>(&_binary_unifont_sfn_start);

	ssfn_dst.ptr = reinterpret_cast<uint8_t*>(main_fb->address);
	ssfn_dst.w = main_fb->width;
	ssfn_dst.h = main_fb->height;
	ssfn_dst.p = main_fb->pitch;
	ssfn_dst.x = ssfn_dst.y = pos.x = pos.y = 0;
	ssfn_dst.fg = fgcolor;
}