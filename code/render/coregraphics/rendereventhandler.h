#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::RenderEventHandler
  
    A render event handler object is notified by the RenderDevice about
    noteworthy events. To react to those events, derive a class from
    RenderEventHandler, and attach to the render device via
    RenderDevice::AttachEventHandler().
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/   
#include "core/refcounted.h"
#include "coregraphics/renderevent.h"

//------------------------------------------------------------------------------
namespace CoreGraphics
{
class RenderEventHandler : public Core::RefCounted
{
    __DeclareClass(RenderEventHandler);
public:
    /// constructor
    RenderEventHandler();
    /// destructor
    virtual ~RenderEventHandler();

    /// called when the event handler is attached to the RenderDevice
    virtual void OnAttach();
    /// called when the event handler is removed from the RenderDevice
    virtual void OnRemove();
    /// called by RenderDevice when an event happens
    virtual bool PutEvent(const RenderEvent& event);

protected:
    /// called when an event should be processed, override this method in your subclass
    virtual bool HandleEvent(const RenderEvent& event);
};

} // namespace CoreGraphics
//------------------------------------------------------------------------------

