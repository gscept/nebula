#pragma once
//------------------------------------------------------------------------------
/**
    A spinlock is a lock which keeps the core busy while the lock is being held.
    Efficiently waits where we can guarantee the acquire/release happens within a short time. 

    Use with caution, as this is not a synchronization primitive, the OS won't 
    be able to yield the CPU core to other threads with the same affinity, which
    may result in groups of cores with narrow affinity masks getting stuck and hung.

    @copyright
    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "threading/interlocked.h"
#include "threading/thread.h"
namespace Threading
{

class Spinlock
{
public:
    /// Constructor
    Spinlock();
    /// Destructor
    ~Spinlock();

    /// Move operator
    void operator=(Spinlock&& rhs);

    /// Lock
    void Lock();
    /// Unlock
    void Unlock();
private:
    volatile Threading::ThreadId lock;
};

//------------------------------------------------------------------------------
/**
*/
inline 
Spinlock::Spinlock()
    : lock(InvalidThreadId)
{
}

//------------------------------------------------------------------------------
/**
*/
inline
Spinlock::~Spinlock()
{
    // Make sure this thread has the lock before we 
    this->Lock();
    this->lock = InvalidThreadId;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
Spinlock::operator=(Spinlock&& rhs)
{
    rhs.Lock();
    rhs.lock = InvalidThreadId;

    // If this wasn't locked when assigned, early out
    if (InvalidThreadId == this->lock)
        return;

    // Lock this thread until whoever is holding it returns the lock
    while (Interlocked::CompareExchange((volatile ThreadIdStorage*)&this->lock, InvalidThreadId, InvalidThreadId) != InvalidThreadId)
    {
        Thread::YieldThread();
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
Spinlock::Lock()
{
    // Attempt to set the lock, if already 1, yield the thread
    ThreadId threadId = Thread::GetMyThreadId();

    // If this thread already owns the spinlock, return early
    if (threadId == this->lock)
        return;

    // Otherwise, enter the spin to exchange the thread id to ours
    while (Interlocked::CompareExchange((volatile ThreadIdStorage*) &this->lock, threadId, InvalidThreadId) != InvalidThreadId)
    {
        Thread::YieldThread();
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
Spinlock::Unlock()
{
    n_assert(this->lock != InvalidThreadId);
    Threading::Interlocked::Exchange((volatile ThreadIdStorage*) &this->lock, InvalidThreadId);
}

struct SpinlockScope
{
    SpinlockScope(Threading::Spinlock* lock)
        : lock(lock)
    {
        this->lock->Lock();
    }
    ~SpinlockScope()
    {
        this->lock->Unlock();
        this->lock = nullptr;
    }

    Threading::Spinlock* lock;
};

} // namespace Threading
