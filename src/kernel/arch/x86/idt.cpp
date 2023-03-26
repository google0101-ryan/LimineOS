#include "idt.h"
#include <kernel/drivers/screen.h>
#include <kernel/arch/x86/util/ports.h>

struct [[gnu::packed]] IDTEntry
{
	uint16_t offset_low;
	uint16_t segment_selector;
	uint8_t ist;
	uint8_t flags;
	uint16_t offset_mid;
	uint32_t offset_high;
	uint32_t reserved;
};

IDTEntry idt[256];

extern "C" uint64_t int_table[256];

void RegisterEntry(int entry, uint64_t addr, uint8_t flags, uint8_t ist)
{
	idt[entry].segment_selector = 0x08;
	idt[entry].offset_low = addr & 0xffff;
	idt[entry].offset_mid = (addr >> 16) & 0xffff;
	idt[entry].offset_high = (addr >> 32) & 0xffffffff;
	idt[entry].ist = ist;
	idt[entry].flags = flags;
	idt[entry].reserved = 0;
}

#define IDT_FLAGS 0x8E // DPL 0, 64-bit interrupt gate, present

struct [[gnu::packed]] IDTDescriptor
{
	uint16_t limit;
	uint64_t offset;
} idtDescriptor;

static const char* exceptions[] =
{
	"Divide-by-zero",
	"Debug",
	"Non-maskable interrupt",
	"Breakpoint",
	"Overflow",
	"Bound range exceeded",
	"Invalid opcode",
	"Device not available",
	"Double fault",
	"Coprocessor segment overrun",
	"Invalid TSS",
	"Segment not present",
	"Stack-segment fault",
	"General protection fault",
	"Page fault",
	"Reserved",
	"x87 floating point",
	"Alignment check",
	"Machine check",
	"SIMD floating point",
	"Virtualization",
	"Control protection",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Hypervisor injection",
	"VMM Communication",
	"Security",
	"Reserved",
};

static IDT::inthandler_t handlers[256];

extern "C" void IntHandler(IDT::stackframe_t* regs)
{
	if (regs->int_num < 32)
	{
		Terminal::SetScreenColors(0xFF0000, 0x00000000);
		printf("Exception %d!\n", regs->int_num);
		printf("%s exception encountered at 0x%x!\n", exceptions[regs->int_num], regs->rip);
		printf("Error code: 0x%x\n", regs->err_code);

		asm volatile("cli");
		for (;;) asm volatile("hlt");
	}
	else if (!handlers[regs->int_num])
	{
		printf("Unhandled interrupt %d!\n", regs->int_num);
		
		asm volatile("cli");
		for (;;) asm volatile("hlt");
	}
	else
		handlers[regs->int_num](regs);
}

#define ICW1_ICW4	0x01		/* ICW4 (not) needed */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */
 
#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */

#define PIC1		0x20		/* IO base address for master PIC */
#define PIC2		0xA0		/* IO base address for slave PIC */
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)

void IDT::Init()
{
	for (int i = 0; i < 256; i++)
		RegisterEntry(i, int_table[i], IDT_FLAGS, 0);
	
	idtDescriptor.limit = (sizeof(IDTEntry)*256) - 1;
	idtDescriptor.offset = (uint64_t)&idt[0];

	outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x00);
    outb(0xA1, 0x00);

	outb(0xA1, 0xFF);
	outb(0x21, 0xFF);

	asm volatile("cli");
	asm volatile("lidt %0" :: "m"(idtDescriptor) : "memory");
	asm volatile("sti");
}

void IDT::LoadIDT()
{
	asm volatile("cli");
	asm volatile("lidt %0" :: "m"(idtDescriptor) : "memory");
	asm volatile("sti");
}

void IDT::AddHandler(int interrupt, inthandler_t handler)
{
	handlers[interrupt] = handler;
}
