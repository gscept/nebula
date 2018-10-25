#pragma once
//------------------------------------------------------------------------------
/**
    @class Win360::Win360ThreadBarrier
    
    Block until all thread have arrived at the barrier.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "threading/criticalsection.h"

//------------------------------------------------------------------------------
namespace Win360
{
class Win360ThreadBarrier
{
public:
    /// constructor
    Win360ThreadBarrier();
    /// destructor
    ~Win360ThreadBarrier();
    /// setup the object with the number of threads
    void Setup(SizeT numThreads);
    /// return true if the object has been setup
    bool IsValid() const;
    /// enter thread barrier, return false if not all threads have arrived yet
    bool Arrive();
    /// call after Arrive() returns false to wait for other threads
    void Wait();
    /// call after Arrive() returns true to resume all threads
    void SignalContinue();

private:
    Threading::CriticalSection critSect;
    long numThreads;
    volatile long outstandingThreads;
    HANDLE event;
    bool isValid;
};

//------------------------------------------------------------------------------
/**
*/
inline
Win360ThreadBarrier::Win360ThreadBarrier() :
    numThreads(0),
    outstandingThreads(0),
    isValid(false)
{
    // create a manual-reset event
    this->event = CreateEvent(NULL, TRUE, FALSE, NULL);
}

//------------------------------------------------------------------------------
/**
*/
inline
Win360ThreadBarrier::~Win360ThreadBarrier()
{
    CloseHandle(this->event);
    this->event = 0;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Win360ThreadBarrier::Setup(SizeT num)
{
    n_assert(!this->isValid);
    this->numThreads = num;
    this->outstandingThreads = num;
    this->isValid = true;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Win360ThreadBarrier::IsValid() const
{
    return this->isValid;
}

//------------------------------------------------------------------------------
/**
    Notify arrival at thread-sync point, return false if not all threads
    have arrived yet, and true if all threads have arrived. If the
    method returns false, you should immediately call Wait(), if the
    method returns true, the caller has a chance to perform some actions
    which should happen before threads continue, and then call the
    SignalContinue() method.
*/
inline bool
Win360ThreadBarrier::Arrive()
{
    this->critSect.Enter();
    n_assert(this->outstandingThreads > 0);
    this->outstandingThreads--;
    return (0 == this->outstandingThreads);
}

//------------------------------------------------------------------------------
/**
    This method should be called when Arrive() returns false. It will
    put the thread to sleep because not all threads have arrived yet.
    When the method returns, all threads have arrived at the sync point.

    NOTE: sometimes both the render and the main thread arrive here
    with the outstandingThreads member set to 1 (from two) causing
    both thread to be waiting idefinitely.
*/
inline void
Win360ThreadBarrier::Wait()
{
    ResetEvent(this->event);
    this->critSect.Leave();
    DWORD reason = WaitForSingleObject(this->event, 2000);
    if (WAIT_TIMEOUT == reason)
    {
        n_printf("Win360ThreadBarrier::Wait() timed out!\n");
    }
}

//------------------------------------------------------------------------------
/**
    This method should be called after Arrive() returns true. This means
    that all threads have arrived at the sync point and execution of all
    threads may resume.
*/
inline void
Win360ThreadBarrier::SignalContinue()
{
    this->outstandingThreads = this->numThreads;
    SetEvent(this->event);
    this->critSect.Leave();
}

} // namespace Win360
//------------------------------------------------------------------------------
    