//------------------------------------------------------------------------------
//  handlerthreadbase.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "messaging/handlerthreadbase.h"
#include "messaging/batchmessage.h"

namespace Messaging
{
__ImplementClass(Messaging::HandlerThreadBase, 'HTBS', Threading::Thread);

using namespace Util;
using namespace Threading;

//------------------------------------------------------------------------------
/**
*/
HandlerThreadBase::HandlerThreadBase()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Attach a message handler to the port. This method may be called from
    any thread.
*/
void
HandlerThreadBase::AttachHandler(const Ptr<Handler>& h)
{
    n_assert(h.isvalid());    
    this->handlersCritSect.Enter();    
    n_assert(InvalidIndex == this->handlers.FindIndex(h));
    this->handlers.Append(h);
    this->handlersCritSect.Leave();
}

//------------------------------------------------------------------------------
/**
    Remove a message handler. This method may be called form any thread.
*/
void
HandlerThreadBase::RemoveHandler(const Ptr<Handler>& h)
{
    n_assert(h.isvalid());

    this->handlersCritSect.Enter();    
    IndexT index = this->handlers.FindIndex(h);
    this->handlers.EraseIndex(index);
    this->handlersCritSect.Leave();
}

//------------------------------------------------------------------------------
/**
    This clears all attached message handlers.
*/
void
HandlerThreadBase::ClearHandlers()
{
    this->handlersCritSect.Enter();
    this->handlers.Clear();
    this->handlersCritSect.Leave();
}

//------------------------------------------------------------------------------
/**
    Wait on the handlers-opened event (will be signalled by the 
    ThreadOpenHandlers method.
*/
void
HandlerThreadBase::WaitForHandlersOpened()
{
    n_assert(this->IsRunning());
    this->handlersOpenedEvent.Wait();
}

//------------------------------------------------------------------------------
/**
    This adds a new message to the thread's message queue. In the base class
    this message is empty and must be implemented in a subclass.
*/
void
HandlerThreadBase::AddMessage(const Ptr<Message>& msg)
{
    // empty, override in subclass!
}

//------------------------------------------------------------------------------
/**
    This removes a message from the thread's message queue, regardless
    of its state. Override this method in a subclass.
*/
void
HandlerThreadBase::CancelMessage(const Ptr<Message>& msg)
{
    // empty, override in subclass!
}

//------------------------------------------------------------------------------
/**
    This waits until the message given as argument has been handled. In 
    order for this message to work, the ThreadSignalMessageHandled() must
    be called from within the handler thread context. Note that 
    subclasses may override this method if they need to.
*/
void
HandlerThreadBase::WaitForMessage(const Ptr<Message>& msg)
{
    while (!msg->Handled())
    {
        this->msgHandledEvent.Wait();
    }
}

//------------------------------------------------------------------------------
/**
    Open attached message handlers. This method must be called at the
    start of the handler thread.
*/
void
HandlerThreadBase::ThreadOpenHandlers()
{
    this->handlersCritSect.Enter();
    Array<Ptr<Handler> >::Iterator iter;
    for (iter = this->handlers.Begin(); iter < this->handlers.End(); iter++)
    {
        (*iter)->Open();
    }
    this->handlersCritSect.Leave();
    this->handlersOpenedEvent.Signal();
}

//------------------------------------------------------------------------------
/**
    Close attached message handlers. This method must be called right
    before the handler thread shuts down.
*/
void
HandlerThreadBase::ThreadCloseHandlers()
{
    this->handlersCritSect.Enter();
    Array<Ptr<Handler> >::Iterator iter;
    for (iter = this->handlers.Begin(); iter < this->handlers.End(); iter++)
    {
        (*iter)->Close();
    }
    this->handlersCritSect.Leave();
}

//------------------------------------------------------------------------------
/**
    Do per-frame update of attached handlers. This will open handlers
    which have been added late, and call the DoWork() method on 
    handlers from within the thread context.
*/
void
HandlerThreadBase::ThreadUpdateHandlers()
{
    this->handlersCritSect.Enter();
    Array<Ptr<Handler> >::Iterator iter;
    for (iter = this->handlers.Begin(); iter < this->handlers.End(); iter++)
    {
        // make sure the handler is open, it may happen that handlers
        // are added as the result of a message, and such a handler
        // may not have been opened yet
        if (!(*iter)->IsOpen())
        {
            (*iter)->Open();
        }
        (*iter)->DoWork();
    }
    this->handlersCritSect.Leave();
}

//------------------------------------------------------------------------------
/**
    This checks every message in the deferred message array whether it
    has been handled yet, if yes, the message's actual handled flag
    will be set, and the message will be removed from the deferred
    handled array. If at least one message has been handled, the method
    will return true, if no message has been handled, the method returns
    false. If message have been handled, don't forget to call
    ThreadSignalMessageHandled() later!
*/
bool
HandlerThreadBase::ThreadUpdateDeferredMessages()
{
    bool msgHandled = false;
    IndexT deferredIndex;
    for (deferredIndex = this->deferredMessages.Size() - 1; deferredIndex != InvalidIndex; deferredIndex--)
    {
        Ptr<Message> msg = this->deferredMessages[deferredIndex];
        if (msg->DeferredHandled())
        {
            this->deferredMessages.EraseIndex(deferredIndex);
            msg->SetHandled(true);
            msgHandled = true;
        }
    }
    return msgHandled;
}

//------------------------------------------------------------------------------
/**
    This clears any leftover deferred messages. Call right before shutdown
    of the handler thread.
*/
void
HandlerThreadBase::ThreadDiscardDeferredMessages()
{
    this->deferredMessages.Clear();
}

//------------------------------------------------------------------------------
/**
    Handle all message in the provided message array. Supports batched
    and deferred messages. Calls ThreadHandleSingleMessage(). If at least
    one message has been handled, the method returns true.
*/
bool
HandlerThreadBase::ThreadHandleMessages(const Array<Ptr<Message> >& msgArray)
{
    this->handlersCritSect.Enter();

    bool msgHandled = false;
    IndexT msgIdx;
    for (msgIdx = 0; msgIdx < msgArray.Size(); ++msgIdx)
    {
        const Ptr<Message>& msg = msgArray[msgIdx];
        // special case: batch message?
        if (msg->CheckId(BatchMessage::Id))
        {
            // process batched messages
            const Array<Ptr<Message> >& msgBatch = msg.cast<BatchMessage>()->GetMessages();
            IndexT msgBatchIndex;
            for (msgBatchIndex = 0; msgBatchIndex < msgBatch.Size(); msgBatchIndex++)
            {
                msgHandled |= this->ThreadHandleSingleMessage(msgBatch[msgBatchIndex]);
            }
            msg->SetHandled(true);
            msgHandled = true;
        }
        else
        {
            // a normal message
            msgHandled |= this->ThreadHandleSingleMessage(msg);
        }
    }
    this->handlersCritSect.Leave();
    return msgHandled;
}

//------------------------------------------------------------------------------
/**
    Handle a single message, called by ThreadHandleMessages(). Return true
    if message has been handled.
    This method MUST be called from ThreadHandleMessages(), since this
    method will not explicitely take the handlers array critical section.
*/
bool
HandlerThreadBase::ThreadHandleSingleMessage(const Ptr<Message>& msg)
{
    bool msgHandled = false;

    // let each handler look at the message
    IndexT i;
    for (i = 0; i < this->handlers.Size(); i++)
    {    
        const Ptr<Handler>& curHandler = this->handlers[i];
        
        // open handler on demand (a handler may have
        // been added as the result of a message call,
        // so let's make sure it will be able to handle 
        // message as soon as possible
        if (!curHandler->IsOpen())
        {
            curHandler->Open();
        }
    
        // let handler handle the current message
        if (curHandler->HandleMessage(msg))
        {
            // message has been accepted by this handler
            if (msg->IsDeferred())
            {
                // message has been defered by the handler
                // and will be handled at some later time,
                // put the message into the deferred queue
                // and handle at a later time
                this->deferredMessages.Append(msg);
            }
            else
            {
                // message has been handled normally
                msg->SetHandled(true);
                msgHandled = true;
            }
            
            // break after the first handler accepts the message
            break;
        }
    }
    return msgHandled;
}

//------------------------------------------------------------------------------
/**
    Signal the message-handled flag. Call this method once per
    handler-loop if either ThreadUpdateDeferredMessages or 
    ThreadHandleMessages returns true!
*/
void
HandlerThreadBase::ThreadSignalMessageHandled()
{
    this->msgHandledEvent.Signal();    
}

} // namespace Messaging