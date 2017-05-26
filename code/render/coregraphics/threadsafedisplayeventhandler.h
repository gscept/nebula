#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::ThreadSafeDisplayEventHandler
  
    A thread-safe subclass of DisplayEventHandler. Allows to receive
    DisplayEvents from a differnet thread then the render thread. The
    producer thread calls the PutEvent() method to push new events into
    the event handler, these events will be stored in a 
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "coregraphics/displayeventhandler.h"
#include "threading/safequeue.h"

//------------------------------------------------------------------------------
namespace CoreGraphics
{
class ThreadSafeDisplayEventHandler : public DisplayEventHandler
{
    __DeclareClass(ThreadSafeDisplayEventHandler);
public:
    /// constructor
    ThreadSafeDisplayEventHandler();
    /// destructor
    virtual ~ThreadSafeDisplayEventHandler();
    /// called by DisplayDevice when an event happens
    virtual bool PutEvent(const DisplayEvent& event);
    /// handle all pending events (called by consumer thread)
    void HandlePendingEvents();

protected:
    /// called when an event should be processed, override this method in your subclass
    virtual bool HandleEvent(const DisplayEvent& event);

    Threading::SafeQueue<DisplayEvent> eventQueue;
    Util::Array<DisplayEvent> tmpPendingEvents;
};

} // namespace CoreGraphics
//------------------------------------------------------------------------------

