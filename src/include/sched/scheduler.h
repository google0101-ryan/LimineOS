#pragma once

#include <include/sched/task.h>
#include <stdint.h>

class Scheduler
{
public:
    static void Initialize();

    static Thread* AddThread(uint64_t entry, bool is_user = true);
    static Thread* AddThread(Thread* t);
    static void Schedule(SavedRegs* regs);

    static void DeleteCurThread();
};