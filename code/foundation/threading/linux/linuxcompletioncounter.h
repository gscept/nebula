#pragma once
//----------------------------------------------------------------------------------------
/**
    @class Linux::LinuxCompletionCounter
    
    Block a thread until count reaches 0.
    
    (C) 2010 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//----------------------------------------------------------------------------------------
namespace Linux
{
class LinuxCompletionCounter
{
public:
    /// constructor
    LinuxCompletionCounter();
    /// destructor
    ~LinuxCompletionCounter();
    
    /// reset the counter, call from main thread
    void Reset(int count);
    /// decrement the counter, call from worker threads, return true if count has reached zero
    bool Decrement(int num);
    /// wait until counter has reached 0, call from main thread or worker threads
    void Wait();
    /// check if the counter has reached 0
    bool Peek();

private:
    volatile int curCount;
    pthread_cond_t completionEvent;
};

//----------------------------------------------------------------------------------------
/**
*/
inline
LinuxCompletionCounter::LinuxCompletionCounter() :
    curCount(0)
{
    // create a manual-reset event, and initially in signalled state
    pthread_cond_init(&this->completionEvent, 0);
}

//----------------------------------------------------------------------------------------
/**
*/
inline
LinuxCompletionCounter::~LinuxCompletionCounter()
{
    pthread_cond_destroy(&this->completionEvent);
}

//----------------------------------------------------------------------------------------
/**
    Reset the counter. Call this method from the main thread before work items
    are pushed to the worker threads. It is safe to use non-thread-safe 
    functions here.
*/
inline void
LinuxCompletionCounter::Reset(int count)
{
    n_assert(count > 0);
    n_assert(0 == this->curCount);
    this->curCount = count;
}

//----------------------------------------------------------------------------------------
/**
    This method is called from several worker thread to decrement the counter.
    When the counter reaches 0, the completionEvent will be signalled and the method 
    will return true, otherwise false.
*/
inline bool
LinuxCompletionCounter::Decrement(int num)
{
    if (LinuxInterlocked::Add(this->curCount, -num) == num)
    {
        pthread_cond_signal(&this->completionEvent);
        return true;
    }
    else
    {
        return false;
    }
}

//----------------------------------------------------------------------------------------
/**
    This method may be called by either one of the worker threads, or the 
    main thread to wait for the completion of an event. Any number of threads
    may wait for completion.
*/
inline void
LinuxCompletionCounter::Wait()
{
    pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&m);
    while(this->curCount > 0) 
    {
        pthread_cond_wait(&this->completionEvent, &m);
    }
    pthread_mutex_unlock(&m);
    pthread_mutex_destroy(&m);
}

//----------------------------------------------------------------------------------------
/**
    Return true if the completion counter has reached 0.
*/
inline bool
LinuxCompletionCounter::Peek()
{
    return (0 == this->curCount);
}

} // namespace Linux
//----------------------------------------------------------------------------------------
