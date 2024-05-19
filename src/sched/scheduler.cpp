#include <include/sched/scheduler.h>
#include <include/limine.h>
#include <include/cpu/cpu.h>
#include <include/cpu/spinlock.h>
#include <include/screen.h>
#include <include/dev/apic.h>
#include <include/mm/pmm.h>

extern limine_smp_request smp_req;
spinlock sched_mutex;
pid_t curPid = 0;
int curCpuCore = 0;

Thread* Scheduler::AddThread(uint64_t entry, bool is_user)
{
    sched_mutex.lock();

    Thread* t = new Thread(entry, is_user);
    t->SetPID(curPid++);

    auto& queue = cpus[curCpuCore].task_queue;

    if (!queue.head)
        queue.head = t;
    
    if (queue.tail)
        queue.tail->next = t;
    queue.tail = t;

    queue.num_threads++;
    // curCpuCore++;

    // if (curCpuCore == smp_req.response->cpu_count)
    //     curCpuCore = 0;

    sched_mutex.unlock();

    return t;
}

Thread *Scheduler::AddThread(Thread *t)
{
    t->SetPID(curPid++);

    auto& queue = cpus[curCpuCore].task_queue;

    if (!queue.head)
        queue.head = t;
    
    if (queue.tail)
        queue.tail->next = t;
    queue.tail = t;

    queue.num_threads++;
    // curCpuCore++;

    // if (curCpuCore == smp_req.response->cpu_count)
    //     curCpuCore = 0;

    sched_mutex.unlock();

    return t;
}

Thread *GetNextThread(uint64_t processor_id)
{
    auto& queue = cpus[processor_id].task_queue;

    if (!queue.num_threads)
        return nullptr;

    Thread* t;

    if (!queue.current)
    {
        queue.current = queue.head;
        t = queue.current;
    }

    if (queue.current->next)
    {
        queue.current = queue.current->next;
        t = queue.current;
    }
    else
    {
        queue.current = queue.head;
        t = queue.current;
    }

    if (!t->CanRun())
        return GetNextThread(processor_id);
    
    return t;
}

void Scheduler::Schedule(SavedRegs *regs)
{
    auto cpu = GetCurCPU(); // Reads from IA32_KERNEL_GS_BASE

    sched_mutex.lock();

    if (cpu->task_queue.current)
        cpu->task_queue.current->ctxt = *regs;

    auto next_thread = GetNextThread(cpu->processor_id);
    
    sched_mutex.unlock();

    if (!next_thread)
        return;

    LAPIC::EOI();

	VirtualMemory::SetCurrentPML4(next_thread->rootTable);

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
				iretq)" :: "r"(&next_thread->ctxt),
				"r"(next_thread->rootTable)
	);
}

void Scheduler::DeleteCurThread()
{
    auto& queue = cpus[GetCurCPU()->processor_id].task_queue;
    queue.current->SetCanRun(false);
}

void Scheduler::Initialize()
{
    sched_mutex.unlock();

    auto& smp_resp = smp_req.response;
    for (int i = 0; i < smp_resp->cpu_count; i++)
    {
        auto id = smp_resp->cpus[i]->processor_id;

        // Make sure the task queue is empty for all CPUs
        cpus[id].task_queue.current = nullptr;
        cpus[id].task_queue.head = nullptr;
        cpus[id].task_queue.tail = nullptr;
        cpus[id].kstack_phys = (void*)PhysicalMemory::alloc(4);
    }
}