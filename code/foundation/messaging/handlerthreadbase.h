#pragma once
//------------------------------------------------------------------------------
/**
    @class Messaging::HandlerThreadBase
    
    Base class for AsyncPort message handler thread classes.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file	
*/
#include "threading/thread.h"
#include "threading/event.h"
#include "threading/criticalsection.h"
#include "messaging/handler.h"
#include "messaging/message.h"

//------------------------------------------------------------------------------
namespace Messaging
{
class HandlerThreadBase : public Threading::Thread
{
    __DeclareClass(HandlerThreadBase);
public:
    /// constructor
    HandlerThreadBase();
    
    /// attach a message handler
    void AttachHandler(const Ptr<Handler>& h);
    /// dynamically remove a message handler
    void RemoveHandler(const Ptr<Handler>& h);
    /// clear all attached message handlers
    void ClearHandlers();
    /// wait until handlers have been opened
    void WaitForHandlersOpened();

    /// add a message to be handled (override in subclass!)
    virtual void AddMessage(const Ptr<Message>& msg);
    /// cancel a pending message (override in subclass!)
    virtual void CancelMessage(const Ptr<Message>& msg);
    /// wait for message to be handled  (optionally override in subclass!)
    virtual void WaitForMessage(const Ptr<Message>& msg);

protected:
    /// open message handlers
    void ThreadOpenHandlers();
    /// close message handlers
    void ThreadCloseHandlers();
    /// open dynamically added handlers, and call DoWork() on all attached handlers
    void ThreadUpdateHandlers();
    /// update deferred messages, return true if at least one message has been handled
    bool ThreadUpdateDeferredMessages();
    /// clear leftover deferred messages
    void ThreadDiscardDeferredMessages();
    /// handle messages in array, return true if at least one message has been handled
    bool ThreadHandleMessages(const Util::Array<Ptr<Message> >& msgArray);
    /// handle a single message without deferred support, return if message has been handled
    bool ThreadHandleSingleMessage(const Ptr<Message>& msg);
    /// signal message handled event (only call if at least one message has been handled)
    void ThreadSignalMessageHandled();

    Threading::Event msgHandledEvent;
    Threading::Event handlersOpenedEvent;
    Threading::CriticalSection handlersCritSect;
    Util::Array<Ptr<Handler> > handlers;
    Util::Array<Ptr<Message> > deferredMessages;
};

} // namespace Messaging
//------------------------------------------------------------------------------
