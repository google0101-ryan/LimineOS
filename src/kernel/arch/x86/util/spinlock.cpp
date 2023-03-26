#include "spinlock.h"

extern "C" int TestAndSet(int newValue, int* ptr);

void mutex_init(mutex_t* m)
{
	m->value = 0;
}

void mutex_lock(mutex_t* m)
{
	while (TestAndSet(1, &m->value) == 1)
		asm volatile("pause");
}

void mutex_unlock(mutex_t* m)
{
	m->value = 0;
}