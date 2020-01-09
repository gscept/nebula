//------------------------------------------------------------------------------
//  rendereventhandler.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/rendereventhandler.h"

namespace CoreGraphics
{
__ImplementClass(CoreGraphics::RenderEventHandler, 'RDEH', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
RenderEventHandler::RenderEventHandler()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
RenderEventHandler::~RenderEventHandler()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RenderEventHandler::OnAttach()
{
    // empty, override in subclass as needed
}

//------------------------------------------------------------------------------
/**
*/
void
RenderEventHandler::OnRemove()
{
    // empty, override in subclass as needed
}

//------------------------------------------------------------------------------
/**
    This method is called by the RenderDevice when an event happens.
    The default behaviour of this class is to call the HandleEvent() method
    directly. Subclasses of RenderEventHandler may choose to implement
    a different behaviour.
*/
bool
RenderEventHandler::PutEvent(const RenderEvent& e)
{
    return this->HandleEvent(e);
}

//------------------------------------------------------------------------------
/**
    Handle a render event. This method is usually called by PutEvent(), but
    subclasses of RenderEventHandler may choose to implement a different
    behaviour. Override this method in your subclass to process the incoming 
    event.
*/
bool
RenderEventHandler::HandleEvent(const RenderEvent& e)
{
    // empty, override in subclass as needed
    return false;
}

} // namespace CoreGraphics