#include <include/screen.h>
#include <include/limine.h>
#include <include/util.h>
#include <include/cpu/spinlock.h>
#include <drivers/font.h>
#include <stdarg.h>

static volatile limine_framebuffer_request fb_req = 
{
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = nullptr,
};

// We mirror whatever is on the screen to the serial terminal
#define PORT_COM0 0x3f8

spinlock screen_lock;

void InitCom0()
{
    Utils::outb(PORT_COM0+1, 0x00);
    Utils::outb(PORT_COM0+3, 0x80);
    Utils::outb(PORT_COM0+0, 0x03);
    Utils::outb(PORT_COM0+1, 0x00);
    Utils::outb(PORT_COM0+3, 0x03);
    Utils::outb(PORT_COM0+2, 0xC7);
    Utils::outb(PORT_COM0+4, 0x0B);
    Utils::outb(PORT_COM0+4, 0x0F);
}

void putc_com0(char c)
{
    Utils::outb(PORT_COM0, c);
}

void puts_com0(const char* str)
{
    while (*str)
        putc_com0(*str++);
}

char* itoa(int64_t val, int base)
{	
	static char buf[64] = {0};
	
	int i = 62;

    if (val == 0)
        return "0";
	
	for(; val && i ; --i, val /= base)
	
		buf[i] = "0123456789abcdef"[val % base];
	
	return &buf[i+1];
	
}

char* utoa(uint64_t val, int base)
{	
	static char buf[64] = {0};
	
	int i = 62;

    if (val == 0)
        return "0";
	
	for(; val && i ; --i, val /= base)
	
		buf[i] = "0123456789abcdef"[val % base];
	
	return &buf[i+1];
	
}

volatile void* fb_address;
limine_framebuffer* chosen_fb;

void PutPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t color = r << chosen_fb->red_mask_shift;
    color |= g << chosen_fb->green_mask_shift;
    color |= b << chosen_fb->blue_mask_shift;
    
    uint64_t fb_index = y * (chosen_fb->pitch / sizeof(uint32_t)) + x;
    uint32_t* fb = (uint32_t*)fb_address;

    fb[fb_index] = color;
}

int x, y;

volatile void *Screen::GetFramebuffer()
{
    return fb_address;
}

void putc(char c)
{
    putc_com0(c);
    if (c == '\n')
    {
        y++;
        x = 0;
    }
    else if (c == '\t')
    {
        x++;
        while ((x % 4) != 0) x++;
    }
    else
    {
        uint8_t* letter = font8x8_basic[c];

        int start_x = x * 8; // 8x8 bitmap font
        int start_y = y * 8;

        int screen_x = start_x;
        int screen_y = start_y;

        for (int y_ = 0; y_ < 8; y_++)
        {
            for (int x_ = 0; x_ < 8; x_++)
            {
                bool set = letter[y_] & (1 << x_);
                if (set)
                {
#ifdef _16x16
                    PutPixel((screen_x*2), (screen_y*2), 255, 255, 255);
                    PutPixel((screen_x*2)+1, (screen_y*2), 255, 255, 255);
                    PutPixel((screen_x*2), (screen_y*2)+1, 255, 255, 255);
                    PutPixel((screen_x*2)+1, (screen_y*2)+1, 255, 255, 255);
#else
                    PutPixel(screen_x, screen_y, 255, 255, 255);
#endif
                }
                else
                {
#ifdef _16x16
                    PutPixel((screen_x*2), (screen_y*2), 0, 0, 0);
                    PutPixel((screen_x*2)+1, (screen_y*2), 0, 0, 0);
                    PutPixel((screen_x*2), (screen_y*2)+1, 0, 0, 0);
                    PutPixel((screen_x*2)+1, (screen_y*2)+1, 0, 0, 0);
#else
                    PutPixel(screen_x, screen_y, 0, 0, 0);
#endif
                }
                screen_x++;
            }
            screen_y++;
            screen_x = start_x;
        }

        x++;
        if (x == chosen_fb->width)
        {
            x = 0;
            y++;
        }
    }
}

int puts(const char* str)
{
    int i = 0;
    while (*str)
    {
        putc(*str++);
        i++;
    }
    return i;
}



int vprintf(const char* format, va_list args)
{
    while (*format)
    {
        if (*format == '%')
        {
            format++;
            switch (*format)
            {
            case 'x':
            {
                uint64_t x = va_arg(args, long);
                puts(utoa(x, 16));
                break;
            }
            case 's':
            {
                char* s = va_arg(args, char*);
                puts(s);
                break;
            }
            case 'd':
            {
                int64_t c = va_arg(args, long);
                puts(itoa(c, 10));
                break;
            }
            case 'u':
            {
                int c = va_arg(args, int);
                puts(utoa(c, 10));
                break;
            }
            case 'c':
            {
                int c = va_arg(args, int);
                putc((char)c);
                break;
            }
            default:
                printf("Unknown format specifier %c\n", *format);
                Utils::HaltCatchFire();
            }
            format++;
        }
        else
        {
            putc(*format++);
        }
    }

    return 0;
}

int _printf(const char* fmt, ...)
{
    screen_lock.lock();
    va_list args;
    va_start(args, fmt);
    int size = vprintf(fmt, args);
    va_end(args);
    screen_lock.unlock();
    return size;
}

void Screen::Initialize()
{
    auto& response = fb_req.response;

    chosen_fb = response->framebuffers[0];

    fb_address = chosen_fb->address;

    printf("Framebuffer is at 0x%x, %dx%d\n", fb_address, chosen_fb->width, chosen_fb->height);
    printf("Framebuffer initialized...\t\t\t\t[OK]\n");

    if (!chosen_fb->edid)
        printf("No EDID detected\n");
}