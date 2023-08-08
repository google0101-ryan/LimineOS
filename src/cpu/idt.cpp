#include <include/cpu/idt.h>

#include <include/screen.h>
#include <include/util.h>

static volatile IDT::IDTEntry idt[256];
static volatile IDT::IDTR idtr;

extern "C" void* isr_table[256];

struct StackFrame
{
    StackFrame* rbp;
    uint64_t rip;
};

void SetIDTEntry(int i, uint64_t entry, uint8_t ist, uint16_t sel, uint8_t gate_type, uint8_t dpl)
{
    idt[i].offset0 = entry & 0xffff;
    idt[i].offset1 = (entry >> 16) & 0xffff;
    idt[i].offset2 = (entry >> 32) & 0xffffffff;
    idt[i].ist = ist;
    idt[i].flags = 0x80 | dpl << 5 | gate_type;
    idt[i].cs = sel;
    idt[i].zero = 0;
}

const char *interrupt_exception_name[] = {
    "Division By 0",
    "Debug interrupt",
    "NMI (Non Maskable Interrupt)",
    "Breakpoint interrupt",
    "invalid (4)", // int 4 is not valid in 64 bit
    "table overflow",
    "Invalid opcode",
    "No FPU",
    "Double fault",
    "invalid (9)", // int 9 is not used
    "invalid TSS",
    "Segment not present",
    "invalid stack",
    "General protection fault",
    "Page fault",
    "invalid (15)",
    "x87 FPU fault",
    "Alignment fault",
    "Machine check fault",
    "SIMD floating point exception",
    "vitualisation excpetion",
    "control protection exception",
    "invalid (22)",
    "invalid (23)",

    "invalid (24)",
    "invalid (25)",
    "invalid (26)",
    "invalid (27)",
    "invalid (28)",
    "invalid (29)",
    "invalid (30)",
    "invalid (31)"};

idt_handler_t handlers[256] =
{
    nullptr
};

extern "C" SavedRegs* interrupt_handler(SavedRegs* regs)
{
    if (regs->int_no < 32)
    {
        printf("PANIC! Exception %s encountered!\n", interrupt_exception_name[regs->int_no]);
        printf("rax 0x%x rbx 0x%x rcx 0x%x rdx 0x%x\n", regs->rax, regs->rbx, regs->rcx, regs->rdx);
        printf("rsi 0x%x rdi 0x%x rbp 0x%x rsp 0x%x\n", regs->rsi, regs->rdi, regs->rbp, regs->rsp);
        printf("r8  0x%x r9  0x%x r10 0x%x r11 0x%x\n", regs->r8, regs->r9, regs->r10, regs->r11);
        printf("r12 0x%x r13 0x%x r14 0x%x r15 0x%x\n", regs->r12, regs->r13, regs->r14, regs->r15);
        printf("Exception occured at 0x%x, error code %d\n", regs->rip, regs->error_code);
        if (regs->int_no == 0x0e)
            printf("cr2 0x%x\n", Utils::ReadCr2());
        
        // printf("Stack trace:\n");
        // StackFrame* stck;
        // asm("movq %%rbp,%0" : "=r"(stck) ::);
        // for (unsigned int frame = 0; stck && frame < 10; ++frame)
        // {
        //     printf("\t0x%x\n", stck->rip);
        //     stck = stck->rbp;
        // }

        Utils::HaltCatchFire();
    }

    if (!handlers[regs->int_no])
    {
        printf("ERROR: Unhandled interrupt %d!\n", regs->int_no);
        Utils::HaltCatchFire();
    }

    return handlers[regs->int_no](regs);
}

extern "C" void LoadIdt(uint64_t idtr);

void IDT::Initialize()
{
    for (int i = 0; i < 256; i++)
        SetIDTEntry(i, (uint64_t)isr_table[i], 0, 0x08, 0xE, 0);

    idtr.offset = (uint64_t)idt;
    idtr.size = sizeof(idt)-1;

    LoadIdt((uint64_t)&idtr);

    asm volatile("sti"); // Please don't triple fault
}

void IDT::AddHandler(int no, idt_handler_t handler)
{
    handlers[no] = handler;
}