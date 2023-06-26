#pragma once

#include <include/sched/task.h>

namespace GDT
{
struct TSS;
}

struct TaskQueue
{
    Thread *head, *tail;
    Thread* current;
    int num_threads = 0;
};

struct CPU
{
    GDT::TSS* tss;
    TaskQueue task_queue;
    int processor_id;
};

extern CPU cpus[256];

CPU* GetCurCPU();