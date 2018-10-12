//------------------------------------------------------------------------------
//  threadsafedisplayeventhandler.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/threadsafedisplayeventhandler.h"

namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ThreadSafeDisplayEventHandler, 'TDEH', CoreGraphics::DisplayEventHandler);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
ThreadSafeDisplayEventHandler::ThreadSafeDisplayEventHandler()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ThreadSafeDisplayEventHandler::~ThreadSafeDisplayEventHandler()
{
    // empty
}    

//------------------------------------------------------------------------------
/**
    Put an event into the event handler. This method is called by the
    render thread's DisplayDevice. Events are queued until the consumer
    thread processes them by calling HandlePendingEvents().
*/
bool
ThreadSafeDisplayEventHandler::PutEvent(const DisplayEvent& e)
{
    this->eventQueue.Enqueue(e);
    // return "event unhandled", because we can't know at this time
    return false;    
}

//------------------------------------------------------------------------------
/**
    Process pending events. This method should be called frequently by
    the consumer thread. Pending events will be deqeued from the 
    internel event queue and the HandleEvent() method will be called
    once per event.
*/
void
ThreadSafeDisplayEventHandler::HandlePendingEvents()
{
    this->eventQueue.DequeueAll(this->tmpPendingEvents);
    IndexT i;
    for (i = 0; i < this->tmpPendingEvents.Size(); i++)
    {
        this->HandleEvent(this->tmpPendingEvents[i]);
    }
}

//------------------------------------------------------------------------------
/**
    Handle an event. This method is called in the consumer thread
    context from the HandlePendingEvents() method for each
    pending event. Override this method in your subclass to process
    the event.
*/
bool
ThreadSafeDisplayEventHandler::HandleEvent(const DisplayEvent& e)
{
    // override in subclass!
    return false;
}

} // namespace CoreGraphics
