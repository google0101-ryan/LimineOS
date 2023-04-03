#pragma once

#include <stdint.h>
#include <kernel/arch/x86/mem/pmm.h>
#include <kernel/arch/x86/mem/vmm.h>
#include <kernel/arch/x86/idt.h>

typedef int pid_t;

class Thread
{
public:
	enum State
	{
		Ready,
		Blocked,
		Running
	};

	IDT::stackframe_t savedRegs;
	VirtualMemory::PageMapLevel4* addressSpace;
	void* kernelStack;
	void* kernelStackBase;
private:
	State state;

	pid_t pid;

	bool usermode;
public:
	Thread* next;

	Thread(uint64_t entry, bool is_user);

	IDT::stackframe_t* GetStackFrame();
	void SetStackFrame(IDT::stackframe_t* regs);

	VirtualMemory::PageMapLevel4* GetAddressSpace();

	void SetState(State state);
	State GetState();

	void SetPID(int id);
	pid_t GetPID();
};