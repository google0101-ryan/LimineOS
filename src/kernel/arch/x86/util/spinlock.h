#pragma once

#include <stdint.h>

struct mutex_t
{
	int value = 0;
};

void mutex_init(mutex_t* m);
void mutex_lock(mutex_t* m);
void mutex_unlock(mutex_t* m);