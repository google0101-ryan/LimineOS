#include "thread.h"
#include <kernel/libk/string.h>

Thread::Thread(uint64_t entry, bool is_user)
{
	uint64_t stack = 0;
	// Allocate a 64 KiB stack
	addressSpace = VirtualMemory::AllocateAddressSpace(stack, 0x10000);
	
	memset(&savedRegs, 0, sizeof(savedRegs));
	
	savedRegs.rip = entry;
	savedRegs.rsp = stack;
	savedRegs.rflags = 0x202;
	savedRegs.ss = is_user ? 0x23 : 0x10;
	savedRegs.cs = is_user ? 0x1B : 0x08;

	next = nullptr;

	state = State::Ready;
}

IDT::stackframe_t* Thread::GetStackFrame()
{
	return &savedRegs;
}

void Thread::SetStackFrame(IDT::stackframe_t *regs)
{
	savedRegs = *regs;
}

void Thread::SetState(State state)
{
	this->state = state;
}

VirtualMemory::PageMapLevel4 *Thread::GetAddressSpace()
{
	return addressSpace;
}

Thread::State Thread::GetState()
{
	return state;
}

void Thread::SetPID(int id)
{
	pid = id;
}

pid_t Thread::GetPID()
{
	return pid;
}
