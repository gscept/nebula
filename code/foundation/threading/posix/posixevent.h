#pragma once
//------------------------------------------------------------------------------
/**
    @class Posix::PosixEvent
 
    Posix implementation of Event. Uses pthread condition variables.
 
    (C) 2010 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

#if __linux__
#include <bits/pthreadtypes.h>
#endif

//------------------------------------------------------------------------------
namespace Posix
{
class PosixEvent
{
public:
    /// constructor
    PosixEvent(bool manualReset=false);
    /// Move constructor
    PosixEvent(PosixEvent&& ev);
    /// destructor
    ~PosixEvent();
    /// signal the event
    void Signal();
    /// wait for the event to become signalled, resets the event
    void Wait() const;
    /// wait for the event with timeout in millisecs, resets the event
    bool WaitTimeout(int ms) const;
    /// check if event is signaled
    bool Peek() const;
    /// manually reset the event
    void Reset();
    /// Returns true if event is manually reset
    bool IsManual() const;

private:
    // emulate windows event behaviour (*sigh*)
    enum EventStatus
    {
        SIGNAL_NONE = 0,
        SIGNAL_ONE = 1,
        SIGNAL_ALL = 2,
    };

    mutable pthread_mutex_t mutex;
    mutable pthread_cond_t cond;
    
    bool manualReset;
    volatile mutable EventStatus status;
};

//------------------------------------------------------------------------------
/**
*/
inline
PosixEvent::PosixEvent(bool manual)
    : manualReset(manual)
    , status(SIGNAL_NONE)
{
    // setup the mutex    
    int res = pthread_mutex_init(&this->mutex, nullptr);
    n_assert(0 == res);

    // setup the condition variable
    res = pthread_cond_init(&this->cond, nullptr);
    n_assert(0 == res);
}

//------------------------------------------------------------------------------
/**
*/
inline 
PosixEvent::PosixEvent(PosixEvent&& ev)
{
    int res = pthread_cond_destroy(&this->cond);
    n_assert(0 == res);
    res = pthread_mutex_destroy(&this->mutex);
    n_assert(0 == res);

    this->mutex = ev.mutex;
    this->cond = ev.cond;
    this->manualReset = ev.manualReset;
    this->status = ev.status;
    ev.mutex = pthread_mutex_t{};
    ev.cond = pthread_cond_t{};
    ev.status = SIGNAL_NONE;
}

//------------------------------------------------------------------------------
/**
*/
inline 
PosixEvent::~PosixEvent()
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
PosixEvent::Signal()
{
    pthread_mutex_lock(&this->mutex);
    if(this->manualReset)
    {
        this->status = SIGNAL_ALL;
        pthread_cond_broadcast(&this->cond);
    }
    else
    {
        this->status = SIGNAL_ONE;
        pthread_cond_signal(&this->cond);
    }
    pthread_mutex_unlock(&this->mutex);
}

//------------------------------------------------------------------------------
/**
*/
inline void
PosixEvent::Wait() const
{
    pthread_mutex_lock(&this->mutex);

    bool locked = false;
    do
    {
        // dont wait for cond if already triggered
        if (this->status == SIGNAL_ONE)
        {
            this->status = SIGNAL_NONE;
            locked = true;
        }
        else if( this->status == SIGNAL_ALL)
        {
            // we are in a manual reset event, do nothing
            locked = true;
        }
        else
        {
            // we actually have to wait
            int res = pthread_cond_wait(&this->cond, &this->mutex);
            n_assert(res == 0);
        }
    } while (!locked);
    pthread_mutex_unlock(&this->mutex);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
PosixEvent::WaitTimeout(int ms) const
{
    bool timeOutOccured = false;
    pthread_mutex_lock(&this->mutex);
    
    bool locked = false;
    do
    {
        // dont wait for cond if already triggered
        if (this->status == SIGNAL_ONE)
        {
            this->status = SIGNAL_NONE;
            locked = true;
        }
        else if( this->status == SIGNAL_ALL)
        {
            // we are in a manual reset event, do nothing
            locked = true;
        }
        else
        {
            timespec timeSpec;
            timeSpec.tv_sec = ms / 1000;
            timeSpec.tv_nsec = (ms - timeSpec.tv_sec * 1000) * 1000000;
        
            int res = pthread_cond_timedwait(&this->cond, &this->mutex, &timeSpec);

            if (ETIMEDOUT == res)
            {
                timeOutOccured = true;
            }
        }
    } while(!locked && !timeOutOccured);
    pthread_mutex_unlock(&this->mutex);    
    return !timeOutOccured;
}
    
//------------------------------------------------------------------------------
/**
*/
inline bool
PosixEvent::Peek() const
{
    return this->status != SIGNAL_NONE;
}

//------------------------------------------------------------------------------
/**
 */
inline void
PosixEvent::Reset()
{
    pthread_mutex_lock(&this->mutex);
    this->status = SIGNAL_NONE;
    pthread_mutex_unlock(&this->mutex);    
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
PosixEvent::IsManual() const
{
    return this->manualReset;
}

} // namespace Posix
//------------------------------------------------------------------------------

