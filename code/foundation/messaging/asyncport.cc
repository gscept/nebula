//------------------------------------------------------------------------------
//  asyncport.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "messaging/asyncport.h"

namespace Messaging
{
__ImplementClass(Messaging::AsyncPort, 'ASPT', Core::RefCounted);

using namespace Util;
using namespace Threading;

//------------------------------------------------------------------------------
/**
*/
AsyncPort::AsyncPort() :
    isOpen(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AsyncPort::~AsyncPort()
{
    if (this->IsOpen())
    {
        this->Close();
    }
    this->thread = nullptr;
}

//------------------------------------------------------------------------------
/**
    Open the async port. The async port needs a valid name before it
    is opened. Messages can only be sent to an open port.
*/
void
AsyncPort::Open()
{
    n_assert(!this->IsOpen());
    n_assert(this->thread.isvalid());

    // start the handler thread, and wait until handlers are open
    this->thread->Start();
    this->thread->WaitForHandlersOpened();

    this->isOpen = true;
}

//------------------------------------------------------------------------------
/**
    Closes the async port.
*/
void
AsyncPort::Close()
{
    n_assert(this->IsOpen());
    this->thread->Stop();
    this->thread->ClearHandlers();
    this->thread = nullptr;
    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
    Add a message handler, this can either be called before the handler
    thread is started, or any time afterwards.
*/
void
AsyncPort::AttachHandler(const Ptr<Handler>& h)
{
    this->thread->AttachHandler(h);
}

//------------------------------------------------------------------------------
/**
    Dynamically remove a message handler.
*/
void
AsyncPort::RemoveHandler(const Ptr<Handler>& h)
{
    this->thread->RemoveHandler(h);
}

//------------------------------------------------------------------------------
/**
    Handle an asynchronous message and return immediately. If the caller
    expects any results from the message he can poll with the AsyncPort::Peek()
    method, or he may wait for the message to be handled with the 
    AsyncPort::Wait() method.
*/
void
AsyncPort::SendInternal(const Ptr<Message>& msg)
{
    n_assert(msg.isvalid());
    n_assert(this->thread.isvalid());
    n_assert(!msg->Handled());
    this->thread->AddMessage(msg);
}

//------------------------------------------------------------------------------
/**
    Send an asynchronous message and wait until the message has been
    handled.
*/
void
AsyncPort::SendWaitInternal(const Ptr<Message>& msg)
{
    this->Send(msg);
    this->Wait(msg);
}

//------------------------------------------------------------------------------
/**
    This method peeks whether a message has already been handled. If the
    caller expects any return arguments from the message handling it 
    can use this message to check whether the results are ready using
    this non-blocking method. The caller can also wait for the results
    to become ready using the Wait() method.
*/
bool
AsyncPort::PeekInternal(const Ptr<Message>& msg)
{
    n_assert(msg.isvalid());
    return msg->Handled();
}

//------------------------------------------------------------------------------
/**
    This method will cancel a pending message.
*/
void
AsyncPort::CancelInternal(const Ptr<Message>& msg)
{
    n_assert(msg.isvalid());
    n_assert(this->thread.isvalid());
    this->thread->CancelMessage(msg);
}

//------------------------------------------------------------------------------
/**
    This method will wait until a message has been handled. If the caller
    expects any return arguments from the message handling it can use
    this method to wait for the results.
*/
void
AsyncPort::WaitInternal(const Ptr<Message>& msg)
{
    n_assert(msg.isvalid());
    n_assert(this->thread.isvalid());

    // only wait if the message hasn't been handled yet
    if (!msg->Handled())
    {
        this->thread->WaitForMessage(msg);
        n_assert(msg->Handled());
    }
}

} // namespace Messaging
