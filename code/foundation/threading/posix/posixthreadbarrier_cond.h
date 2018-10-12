#pragma once
//------------------------------------------------------------------------------
/**
    @class Posix::PosixThreadBarrier
    
    Block until all thread have arrived at the barrier.
    
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "threading/criticalsection.h"
#include <pthread.h>
#include <time.h>

//------------------------------------------------------------------------------
namespace Posix
{
class PosixThreadBarrier
{
public:
    /// constructor
    PosixThreadBarrier();
    /// destructor
    ~PosixThreadBarrier();
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
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    bool isValid;
    timespec ts;
};

//------------------------------------------------------------------------------
/**
*/
inline
PosixThreadBarrier::PosixThreadBarrier() :
    numThreads(0),
    outstandingThreads(0),    
    isValid(false)
{
    pthread_cond_init(&cond,NULL);
    pthread_mutex_init(&mutex,NULL);
}

//------------------------------------------------------------------------------
/**
*/
inline
PosixThreadBarrier::~PosixThreadBarrier()
{
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
}

//------------------------------------------------------------------------------
/**
*/
inline void
PosixThreadBarrier::Setup(SizeT num)
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
PosixThreadBarrier::IsValid() const
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
PosixThreadBarrier::Arrive()
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
PosixThreadBarrier::Wait()
{
    pthread_mutex_lock(&mutex);
    this->critSect.Leave();
    clock_gettime(CLOCK_REALTIME,&ts);
    ts.tv_sec += 2;
    int reason = pthread_cond_timedwait(&cond,&mutex,&ts);    
    if ( ETIMEDOUT == reason )
    {
        n_printf("PosixThreadBarrier::Wait() timed out!\n");
    }
}

//------------------------------------------------------------------------------
/**
    This method should be called after Arrive() returns true. This means
    that all threads have arrived at the sync point and execution of all
    threads may resume.
*/
inline void
PosixThreadBarrier::SignalContinue()
{
    this->outstandingThreads = this->numThreads;
    pthread_cond_broadcast(&cond);    
    this->critSect.Leave();
}

} // namespace Posix
//------------------------------------------------------------------------------
    