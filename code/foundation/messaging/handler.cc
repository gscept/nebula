//------------------------------------------------------------------------------
//  handler.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "messaging/handler.h"

namespace Messaging
{
__ImplementClass(Messaging::Handler, 'MHDL', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
Handler::Handler() :
    isOpen(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Handler::~Handler()
{
    n_assert(!this->IsOpen());
}

//------------------------------------------------------------------------------
/**
    Open the handler. This method is called once after the handler has been
    attached to a port and before the first call to HandleMessage().
*/
void
Handler::Open()
{
    n_assert(!this->IsOpen());
    this->isOpen = true;
}

//------------------------------------------------------------------------------
/**
    Close the handler. This method is called once before the handler
    is detached from the port.
*/
void
Handler::Close()
{
    n_assert(this->IsOpen());
    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
    Derive this method in a subclass to handle specific messages. The method
    should return true only if a message has been handled.
*/
bool
Handler::HandleMessage(const Ptr<Message>& msg)
{
    n_assert(msg.isvalid());
    n_assert(this->isOpen);
    return false;
}

//------------------------------------------------------------------------------
/**
    This is an optional method for handlers which need to do continuous
    work (like a render thread message handler). This message will
    be called after messages have been handled.
*/
void
Handler::DoWork()
{
    // empty, override in subclass
    n_assert2(this->isOpen, this->GetRtti()->GetName().AsCharPtr());
}

} // namespace Messaging
