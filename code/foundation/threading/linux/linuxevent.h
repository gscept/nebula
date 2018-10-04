#pragma once
//------------------------------------------------------------------------------
/**
    @class Linux::LinuxEvent
 
    Linux implementation of Event. Uses pthread condition variables.
 
    (C) 2010 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include <bits/pthreadtypes.h>

//------------------------------------------------------------------------------
namespace Linux
{
class LinuxEvent
{
public:
    /// constructor
    LinuxEvent(bool manualReset=false);
    /// destructor
    ~LinuxEvent();
    /// signal the event
    void Signal();
    /// wait for the event to become signalled, resets the event
    void Wait() const;
    /// wait for the event with timeout in millisecs, resets the event
    bool WaitTimeout(int ms) const;
    /// check if event is signalled
    bool Peek() const;
    /// manually reset the event
    void Reset();

private:
    mutable pthread_mutex_t mutex;
    mutable pthread_cond_t cond;
    mutable volatile int signalled;
    bool manualReset;
};

//------------------------------------------------------------------------------
/**
*/
inline
LinuxEvent::LinuxEvent(bool manual) :
    signalled(0),
    manualReset(manual)
{
    // setup the mutex
    pthread_mutexattr_t mutexAttrs;
    pthread_mutexattr_init(&mutexAttrs);
    int res = pthread_mutex_init(&this->mutex, &mutexAttrs);
    n_assert(0 == res);
    pthread_mutexattr_destroy(&mutexAttrs);

    // setup the condition variable
    pthread_condattr_t condAttrs;
    pthread_condattr_init(&condAttrs);
    res = pthread_cond_init(&this->cond, &condAttrs);
    n_assert(0 == res);
    pthread_condattr_destroy(&condAttrs);
}

//------------------------------------------------------------------------------
/**
*/
inline 
LinuxEvent::~LinuxEvent()
{
    int res = pthread_cond_destroy(&this->cond);
    n_assert(0 == res);
    res = pthread_mutex_destroy(&this->mutex);
    n_assert(0 == res);
}
    
//------------------------------------------------------------------------------
/**
*/
inline void
LinuxEvent::Signal()
{
    pthread_mutex_lock(&this->mutex);    
    __sync_lock_test_and_set(&this->signalled, 1);
    pthread_cond_signal(&this->cond);
    pthread_mutex_unlock(&this->mutex);
}

//------------------------------------------------------------------------------
/**
*/
inline void
LinuxEvent::Wait() const
{
    pthread_mutex_lock(&this->mutex);
    while (!this->signalled)
    {
        pthread_cond_wait(&this->cond, &this->mutex);
    }
    if(!this->manualReset)
    {
        __sync_lock_test_and_set(&this->signalled, 0);
    }
    pthread_mutex_unlock(&this->mutex);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
LinuxEvent::WaitTimeout(int ms) const
{
    bool timeOutOccured = false;
    pthread_mutex_lock(&this->mutex);

    while (!this->signalled)
    {
        // TODO: needs testing
        /*
        timeval timeVal;
        gettimeofday(&timeVal, 0);

        timespec timeSpec;
        timeSpec.tv_sec = timeVal.tv_sec;
        timeSpec.tv_nsec = timeVal.tv_usec * 1000 + ms * 1000000;

        timeSpec.tv_sec += timeSpec.tv_nsec/1000000000;
        timeSpec.tv_nsec += timeSpec.tv_nsec%1000000000;        

        int res = pthread_cond_timedwait(&this->cond, &this->mutex, &timeSpec);
        */
        
        timespec timeSpec;
        timeSpec.tv_sec = ms / 1000;
        timeSpec.tv_nsec = (ms % 1000) * 1000000;
		#if __NACL__
        	timeval tv;
        	gettimeofday(&tv, 0);
        	timeSpec.tv_sec += tv.tv_sec;
        	timeSpec.tv_nsec += tv.tv_usec * 1000;
        	int res = pthread_cond_timedwait_abs(&this->cond, &this->mutex, &timeSpec);
		#elif __OSX__
        	int res = pthread_cond_timedwait_relative_np(&this->cond, &this->mutex, &timeSpec);
        #elif __LINUX__
            int res = pthread_cond_timedwait(&this->cond, &this->mutex, &timeSpec);
		#endif
        if (ETIMEDOUT == res)
        {
            timeOutOccured = true;
            break;
        }
    }
    if (!this->manualReset)
    {
        __sync_lock_test_and_set(&this->signalled, 0);
    }
    pthread_mutex_unlock(&this->mutex);    
    return !timeOutOccured;
}
    
//------------------------------------------------------------------------------
/**
*/
inline bool
LinuxEvent::Peek() const
{
    return this->signalled;
}

//------------------------------------------------------------------------------
/**
 */
inline void
LinuxEvent::Reset()
{
    pthread_mutex_lock(&this->mutex);
    __sync_lock_test_and_set(&this->signalled, 0);    
    pthread_mutex_unlock(&this->mutex);    
}

} // namespace Linux
//------------------------------------------------------------------------------

