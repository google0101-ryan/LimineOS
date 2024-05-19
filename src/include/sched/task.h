#pragma once

#include <include/cpu/idt.h>
#include <include/mm/vmm.h>

typedef int pid_t;

class Thread
{
public:
    SavedRegs ctxt;
    PML4Table* rootTable;
    pid_t pid;
    bool canRun = true;

    Thread* next; // Used to make a linked list of tasks
public:
    Thread(uint64_t entryPoint, bool is_user = true);

    void MapPage(void* phys, uint64_t virt, uint64_t flags);

    void SetPID(pid_t p) {pid = p;}

    void SetCanRun(bool runnable) {canRun = runnable;}
    bool CanRun() {return canRun;}
};