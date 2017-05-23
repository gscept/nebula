#pragma once
#ifndef POSIX_POSIXEVENT_H
#define POSIX_POSIXEVENT_H
//------------------------------------------------------------------------------
/**
    @class Posix::PosixEvent

    Posix implmentation of an event synchronization object.

    (C) 2006 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include <semaphore.h>

//------------------------------------------------------------------------------
namespace Posix
{
class PosixEvent
{
public:
    /// constructor
    PosixEvent(bool manualReset=false);
    /// destructor
    ~PosixEvent();
    /// signal the event
    void Signal();
    /// reset the event (only if manual reset)
    void Reset();
    /// wait for the event to become signalled
    void Wait() const;
    /// wait for the event with timeout in millisecs
    bool WaitTimeout(int ms) const;
    /// check if event is signalled
    bool Peek() const;
private:
    sem_t* semaphore;
};

//------------------------------------------------------------------------------
/**
    manual reset is not used, since it's only used for win32events
*/
inline
PosixEvent::PosixEvent(bool manualReset) : 
    semaphore(0)
{
    this->semaphore = new sem_t;
    sem_init(this->semaphore, 0, 0);
}

//------------------------------------------------------------------------------
/**
*/
inline
PosixEvent::~PosixEvent()
{
    sem_destroy(this->semaphore);
    delete this->semaphore;
    this->semaphore = 0;
}

//------------------------------------------------------------------------------
/**
*/
inline void
PosixEvent::Signal()
{
    sem_post(this->semaphore);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
PosixEvent::Reset()
{
    // do nothing, not required for POSIX events
}

//------------------------------------------------------------------------------
/**
*/
inline void
PosixEvent::Wait() const
{
    sem_wait(this->semaphore);
}

//------------------------------------------------------------------------------
/**
    Waits for the event to become signaled with a specified timeout
    in milli seconds. If the method times out it will return false,
    if the event becomes signalled within the timeout it will return 
    true.
*/
inline bool
PosixEvent::WaitTimeout(int timeoutInMilliSec) const
{
    while (timeoutInMilliSec > 0)
    {
        if (0 == sem_trywait(this->semaphore))
        {
            return true;
        }
        usleep(1000);
        timeoutInMilliSec -= 1;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    This checks if the event is signalled and returnes immediately.
*/
inline bool
PosixEvent::Peek() const
{
    return 0 == sem_trywait(this->semaphore);
}

}; // namespace Posix
//------------------------------------------------------------------------------
#endif

