#pragma once
//------------------------------------------------------------------------------
/**
    @class Messaging::BatchMessage
  
    A batch of messages which is itself a message. Use batch messaging if you
    want to reduce thread synchronization when sending many messages through
    an AsyncPort. Instead batch many messages into a single batch message
    (which doesn't require thread synchronization), then send the batch
    message as one into the AsyncPort.

    Note that the following features don't work on batched messages:

    - waiting for the message to become handled
    - cancelling the message
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/    
#include "messaging/message.h"
#include "util/array.h"

//------------------------------------------------------------------------------
namespace Messaging
{
class BatchMessage : public Message
{
    __DeclareClass(BatchMessage);
    __DeclareMsgId;
public:
    /// constructor
    BatchMessage();    
    /// add a message
    void AddMessage(const Ptr<Message>& msg);
    /// read access to message array
    const Util::Array<Ptr<Message> >& GetMessages() const;

private:
    Util::Array<Ptr<Message> > messages;
};

} // namespace Messaging
//------------------------------------------------------------------------------
