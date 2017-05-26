#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::ThreadSafeRenderEventHandler
    
    A thread-safe subclass of RenderEventHandler. Allows to receive
    RenderEvents from a differnet thread then the render thread. The
    producer thread calls the PutEvent() method to push new events into
    the event handler, these events will be stored in a 
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "coregraphics/rendereventhandler.h"
#include "threading/safequeue.h"

//------------------------------------------------------------------------------
namespace CoreGraphics
{
class ThreadSafeRenderEventHandler : public RenderEventHandler
{
    __DeclareClass(ThreadSafeRenderEventHandler);
public:
    /// constructor
    ThreadSafeRenderEventHandler();
    /// destructor
    virtual ~ThreadSafeRenderEventHandler();
    /// called by RenderDevice when an event happens
    virtual bool PutEvent(const RenderEvent& event);
    /// handle all pending events (called by consumer thread)
    void HandlePendingEvents();

protected:
    /// called when an event should be processed, override this method in your subclass
    virtual bool HandleEvent(const RenderEvent& event);

    Threading::SafeQueue<RenderEvent> eventQueue;
    Util::Array<RenderEvent> tmpPendingEvents;
};

} // namespace CoreGraphics
//------------------------------------------------------------------------------

