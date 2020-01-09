#pragma once
//------------------------------------------------------------------------------
/**
    @class Messaging::RunThroughHandlerThread
    
    A simple handler thread class which "runs thru", and doesn't wait for
    messages. This is the "old behaviour" of the N3 render thread.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "messaging/handlerthreadbase.h"
#include "threading/safequeue.h"

//------------------------------------------------------------------------------
namespace Messaging
{
class RunThroughHandlerThread : public HandlerThreadBase
{
    __DeclareClass(RunThroughHandlerThread);
public:
    /// constructor
    RunThroughHandlerThread();

    /// add a message to be handled (override in subclass!)
    virtual void AddMessage(const Ptr<Message>& msg);
    /// cancel a pending message (override in subclass!)
    virtual void CancelMessage(const Ptr<Message>& msg);

    /// this method runs in the thread context
    virtual void DoWork();

private:
    Threading::SafeQueue<Ptr<Message> > msgQueue;
};

} // namespace Messaging
//------------------------------------------------------------------------------
