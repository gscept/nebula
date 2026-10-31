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
    /// Move constructor
    LinuxEvent(LinuxEvent&& ev);
    /// copy constructor
    LinuxEvent(const LinuxEvent& ev) = default;
    /// destructor
    ~LinuxEvent();
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

    /// copy assignment operator
    LinuxEvent& operator=(const LinuxEvent& ev) = default;

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
LinuxEvent::LinuxEvent(bool manual)
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
LinuxEvent::LinuxEvent(LinuxEvent&& ev)
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
LinuxEvent::Wait() const
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
LinuxEvent::WaitTimeout(int ms) const
{
    bool timeOutOccured = false;

    // compute the absolute deadline once, so spurious wakeups don't extend the timeout,
    // and keep tv_nsec below one second, otherwise pthread_cond_timedwait() fails with
    // EINVAL and we would busy-spin until the clock produces a valid deadline
    timespec target;
    clock_gettime(CLOCK_REALTIME, &target);
    target.tv_sec += ms / 1000;
    target.tv_nsec += (ms % 1000) * 1000000L;
    if (target.tv_nsec >= 1000000000L)
    {
        target.tv_sec += 1;
        target.tv_nsec -= 1000000000L;
    }

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
            int res = pthread_cond_timedwait(&this->cond, &this->mutex, &target);
            if (ETIMEDOUT == res)
            {
                timeOutOccured = true;
            }
            else
            {
                n_assert(res == 0);
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
LinuxEvent::Peek() const
{
    return this->status != SIGNAL_NONE;
}

//------------------------------------------------------------------------------
/**
 */
inline void
LinuxEvent::Reset()
{
    pthread_mutex_lock(&this->mutex);
    this->status = SIGNAL_NONE;
    pthread_mutex_unlock(&this->mutex);    
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
LinuxEvent::IsManual() const
{
    return this->manualReset;
}

} // namespace Linux
//------------------------------------------------------------------------------

