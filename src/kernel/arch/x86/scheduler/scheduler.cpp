#include "scheduler.h"
#include <kernel/arch/x86/util/spinlock.h>
#include <kernel/arch/x86/driver/apic.h>
#include <kernel/arch/x86/cpu.h>
#include <kernel/arch/x86/gdt.h>

static mutex_t sched_mutex;

struct
{
	Thread* current;
	Thread* head;
	Thread* tail;
	int num_threads = 0;
} thread_queue;

bool schedule = false;

// WARNING! Only the BSP should call this!
void Scheduler::Init()
{
	mutex_init(&sched_mutex);

	thread_queue.current = nullptr;
	thread_queue.head = nullptr;
	thread_queue.tail = nullptr;

	schedule = true;
}

static pid_t curPid = 0;

Thread *Scheduler::AddThread(uint64_t entrypoint, bool user)
{
	mutex_lock(&sched_mutex);

	auto ret = new Thread(entrypoint, user);
	ret->SetPID(curPid++);

	if (!thread_queue.head)
		thread_queue.head = ret;
	
	if (thread_queue.tail)
		thread_queue.tail->next = ret;
	thread_queue.tail = ret;

	thread_queue.num_threads++;

	mutex_unlock(&sched_mutex);

	return ret;
}

void Scheduler::RegisterThread(Thread *thread)
{
	mutex_lock(&sched_mutex);

	if (!thread_queue.head)
		thread_queue.head = thread;
	
	if (thread_queue.tail)
		thread_queue.tail->next = thread;
	thread_queue.tail = thread;

	thread_queue.num_threads++;

	mutex_unlock(&sched_mutex);
}

Thread *Scheduler::GetCurrentThread()
{
	return thread_queue.current;
}

bool firsttime = true;

Thread* GetNextReadyThread()
{
	static int threads_searched = 0;

	if (firsttime)
		threads_searched = 0;
	
	if (threads_searched == thread_queue.num_threads)
	{
		return nullptr;
	}

	if (!thread_queue.current)
	{
		thread_queue.current = thread_queue.head;
		return thread_queue.current;
	}

	if (thread_queue.current->next)
	{
		thread_queue.current = thread_queue.current->next;
		if (thread_queue.current->GetState() != Thread::Ready)
		{
			threads_searched++;
			return GetNextReadyThread();
		}
		firsttime = true;
		return thread_queue.current;
	}
	else
	{
		thread_queue.current = thread_queue.head;
		if (thread_queue.current->GetState() != Thread::Ready)
		{
			threads_searched++;
			return GetNextReadyThread();
		}
		firsttime = true;
		return thread_queue.current;
	}
}

void Scheduler::Schedule(IDT::stackframe_t* regs)
{
	if (!schedule)
		return;
	
	if (!thread_queue.head) // No threads in queue
		return;

	mutex_lock(&sched_mutex);

	thread_queue.current->SetStackFrame(regs);
	thread_queue.current->SetState(Thread::Ready);

	auto next_thread = GetNextReadyThread();

	if (!next_thread)
		return;

	next_thread->SetState(Thread::Running);
	
	GDT::SetKernelStack((uint64_t)next_thread->kernelStack);

	lapic.EOI();

	mutex_unlock(&sched_mutex);

	asm volatile(
		R"(mov %0, %%rsp;
				mov %1, %%rax;
				pop %%r15;
				pop %%r14;
				pop %%r13;
				pop %%r12;
				pop %%r11;
				pop %%r10;
				pop %%r9;
				pop %%r8;
				pop %%rbp;
				pop %%rdi;
				pop %%rsi;
				pop %%rdx;
				pop %%rcx;
				pop %%rbx;
				
				mov %%rax, %%cr3
				pop %%rax
				addq $16, %%rsp
				iretq)" :: "r"(&next_thread->savedRegs),
				"r"(next_thread->addressSpace)
	);
}
