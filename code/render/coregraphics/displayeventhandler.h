#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::DisplayEventHandler
  
    A display event handler object is notified by the DisplayDevice about
    noteworthy window events, for instance when the mouse is moved, the
    window is minimized, and so on. To get notified about those events,
    derive a class from DisplayEventHandler and attach to the display
    device via DisplayDevice::AttachEventHandler().
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "coregraphics/displayevent.h"

//------------------------------------------------------------------------------
namespace CoreGraphics
{
class DisplayEventHandler : public Core::RefCounted
{
    __DeclareClass(DisplayEventHandler);
public:
    /// constructor
    DisplayEventHandler();
    /// destructor
    virtual ~DisplayEventHandler();

    /// called when the event handler is attached to the DisplayDevice
    virtual void OnAttach();
    /// called when the event handler is removed from the DisplayDevice
    virtual void OnRemove();
    /// called by DisplayDevice when an event happens
    virtual bool PutEvent(const DisplayEvent& event);

protected:
    /// called when an event should be processed, override this method in your subclass
    virtual bool HandleEvent(const DisplayEvent& event);
};

} // namespace CoreGraphics
//------------------------------------------------------------------------------

