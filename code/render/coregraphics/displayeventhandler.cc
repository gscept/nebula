//------------------------------------------------------------------------------
//  displayeventhandler.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/displayeventhandler.h"

namespace CoreGraphics
{
__ImplementClass(CoreGraphics::DisplayEventHandler, 'DSEH', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
DisplayEventHandler::DisplayEventHandler()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
DisplayEventHandler::~DisplayEventHandler()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
DisplayEventHandler::OnAttach()
{
    // empty, override in subclass as needed
}

//------------------------------------------------------------------------------
/**
*/
void
DisplayEventHandler::OnRemove()
{
    // empty, override in subclass as needed
}

//------------------------------------------------------------------------------
/**
    This method is called by the DisplayDevice when an event happens.
    The default behaviour of this class is to call the HandleEvent() method
    directly. Subclasses of DisplayEventHandler may choose to implement
    a different behaviour.
*/
bool
DisplayEventHandler::PutEvent(const DisplayEvent& e)
{
    return this->HandleEvent(e);
}

//------------------------------------------------------------------------------
/**
    Handle a display event. This method is usually called by PutEvent(), but
    subclasses of DisplayEventHandler may choose to implement a different
    behaviour. Override this method in your subclass to process the incoming 
    event.
*/
bool
DisplayEventHandler::HandleEvent(const DisplayEvent& e)
{
    // empty, override in subclass as needed
    return false;
}

} // namespace CoreGraphics

