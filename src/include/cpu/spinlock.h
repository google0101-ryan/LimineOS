#include "atomic.h"

struct spinlock
{
public:
    spinlock() : value_(0) {}

    void lock()
    {
        while (!value_.compare_exchange(0, 1))
            ;
    }

    void unlock() {value_.store(0);}
private:
    atomic::atomic<int> value_;

    ATOMIC_DISALLOW_COPY(spinlock);
};