#pragma once 
//------------------------------------------------------------------------------
/**
    @class Posix::PosixThreadBarrier
    
    Block until all thread have arrived at the barrier.
    
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "threading/criticalsection.h"
#include <semaphore.h>

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
    sem_t* semaphore;
    bool isValid;
};

//------------------------------------------------------------------------------
/**
*/
PosixThreadBarrier::PosixThreadBarrier() :
    numThreads(0),
    outstandingThreads(0),
    isValid(false)
{
    this->semaphore = new sem_t;
    sem_init(this->semaphore, 0, 0);
}

//------------------------------------------------------------------------------
/**
*/
PosixThreadBarrier::~PosixThreadBarrier()
{
    sem_destroy(this->semaphore);
    delete this->semaphore;
    this->semaphore = 0;
}

//------------------------------------------------------------------------------
/**
*/
void 
PosixThreadBarrier::Setup(SizeT numThreads)
{
    n_assert(!this->IsValid());
    this->numThreads = numThreads;
    this->outstandingThreads = numThreads;
    this->isValid = true;
}

//------------------------------------------------------------------------------
/**
*/
bool 
PosixThreadBarrier::IsValid() const
{
    return this->isValid;
}

//------------------------------------------------------------------------------
/**
*/
bool 
PosixThreadBarrier::Arrive()
{
    this->critSect.Enter();
    n_assert(this->outstandingThreads > 0);
    this->outstandingThreads--;
    return (0 == this->outstandingThreads);
}

//------------------------------------------------------------------------------
/**
*/
void 
PosixThreadBarrier::Wait()
{
    this->critSect.Leave();
    int timeout = 2000;
    while (timeout > 0)
    {
        if (0 == sem_trywait(this->semaphore))
        {
            return;
        }
        usleep(1000);
        timeout -= 1;
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
PosixThreadBarrier::SignalContinue()
{
    this->outstandingThreads = this->numThreads;
    sem_post(this->semaphore);
    this->critSect.Leave();
}


} // namespace PosiX
