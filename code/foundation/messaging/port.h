#pragma once
//------------------------------------------------------------------------------
/**
    @class Messaging::Port

    A message port is a receiving point for messages. Messages processed 
    immediately and the port will be blocked until the message has been 
    processed.

    Messages are processed by message handlers which are attached to the
    port. More then one message handler can be attached to a port. When
    a message should be attached, each message handler is called in their
    attachment order until one of the handlers returns true, which means
    that the message has been handled.

    For an asynchronous port implementation, which runs the message
    handlers in a separate thread, please check Message::AsyncPort.

    (C) 2006 RadonLabs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/ptr.h"
#include "messaging/message.h"
#include "messaging/handler.h"

//------------------------------------------------------------------------------
namespace Messaging
{
class Port : public Core::RefCounted
{
    __DeclareClass(Port);
public:
    /// override to register accepted messages
    virtual void SetupAcceptedMessages();
    /// attach a message handler to the port
    void AttachHandler(const Ptr<Handler>& h);
    /// remove a message handler from the port
    void RemoveHandler(const Ptr<Handler>& h);
    /// remove all message handler from the port
    void RemoveAllHandlers();
    /// return number of handlers attached to the port
    SizeT GetNumHandlers() const;
    /// get a message handler by index
    const Ptr<Handler>& GetHandlerAtIndex(IndexT i) const;
    /// send a message to the port
    virtual void Send(const Ptr<Message>& msg);
    /// get the array of accepted messages (sorted)
    const Util::Array<const Id*>& GetAcceptedMessages() const;    
    /// return true if port accepts this msg
    bool AcceptsMessage(const Id& msgId) const;
    /// handle a single accepted message
    virtual void HandleMessage(const Ptr<Messaging::Message>& msg);

protected:
    /// register a single accepted message
    void RegisterMessage(const Id& msgId);

private:
    Util::Array<Ptr<Handler> > handlers;
    Util::Array<const Id*> acceptedMessageIds;
};

//------------------------------------------------------------------------------
/**
*/
inline
SizeT
Port::GetNumHandlers() const
{
    return this->handlers.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
Port::RegisterMessage(const Id& msgId)
{
    // ignore duplicate message ids (may happen when derived classes
    // process the same messages)
    if (InvalidIndex == this->acceptedMessageIds.BinarySearchIndex(&msgId))
    {
        this->acceptedMessageIds.InsertSorted(&msgId);
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::Array<const Id*>&
Port::GetAcceptedMessages() const
{
    return this->acceptedMessageIds;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Ptr<Handler>&
Port::GetHandlerAtIndex(IndexT i) const
{
    return this->handlers[i];
}

//------------------------------------------------------------------------------
/**
*/
inline
bool 
Port::AcceptsMessage(const Id& msgId) const
{
    return (InvalidIndex != this->acceptedMessageIds.FindIndex(&msgId));
}

} // namespace Messaging
//------------------------------------------------------------------------------
    