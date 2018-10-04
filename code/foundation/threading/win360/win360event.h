#pragma once
//------------------------------------------------------------------------------
/**
    @class Win360::Win360Event

    Win32/Xbox360 implmentation of an event synchronization object.

    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file	
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Win360
{
class Win360Event
{
public:
    /// constructor
    Win360Event(bool manualReset=false);
    /// destructor
    ~Win360Event();
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
    HANDLE event;
};

//------------------------------------------------------------------------------
/**
*/
inline
Win360Event::Win360Event(bool manualReset)
{
    this->event = CreateEvent(NULL, manualReset, FALSE, NULL);
    n_assert(0 != this->event);
}

//------------------------------------------------------------------------------
/**
*/
inline
Win360Event::~Win360Event()
{
    CloseHandle(this->event);
    this->event = 0;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Win360Event::Signal()
{
    SetEvent(this->event);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Win360Event::Reset()
{
    ResetEvent(this->event);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Win360Event::Wait() const
{
    WaitForSingleObject(this->event, INFINITE);
}

//------------------------------------------------------------------------------
/**
    Waits for the event to become signaled with a specified timeout
    in milli seconds. If the method times out it will return false,
    if the event becomes signalled within the timeout it will return 
    true.
*/
inline bool
Win360Event::WaitTimeout(int timeoutInMilliSec) const
{
    DWORD res = WaitForSingleObject(this->event, timeoutInMilliSec);
    return (WAIT_TIMEOUT == res) ? false : true;
}

//------------------------------------------------------------------------------
/**
    This checks if the event is signalled and returnes immediately.
*/
inline bool
Win360Event::Peek() const
{
    DWORD res = WaitForSingleObject(this->event, 0);
    return (WAIT_TIMEOUT == res) ? false : true;
}

} // namespace Win360
//------------------------------------------------------------------------------

