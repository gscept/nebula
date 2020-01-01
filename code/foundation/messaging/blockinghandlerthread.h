#pragma once
//------------------------------------------------------------------------------
/**
    @class Messaging::BlockingHandlerThread
  
    Message handler thread class which blocks until messages arrive
    (or optionally, a time-out occurs).
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "messaging/handlerthreadbase.h"
#include "threading/safequeue.h"

//------------------------------------------------------------------------------
namespace Messaging
{
class BlockingHandlerThread : public HandlerThreadBase
{
    __DeclareClass(BlockingHandlerThread);
public:
    /// constructor
    BlockingHandlerThread();

    /// set optional wait timeout (0 if infinite)
    void SetWaitTimeout(int milliSec);
    /// get wait timeout
    int GetWaitTimeout() const;

    /// add a message to be handled (override in subclass!)
    virtual void AddMessage(const Ptr<Message>& msg);
    /// cancel a pending message (override in subclass!)
    virtual void CancelMessage(const Ptr<Message>& msg);

    /// called if thread needs a wakeup call before stopping
    virtual void EmitWakeupSignal();
    /// this method runs in the thread context
    virtual void DoWork();

private:
    int waitTimeout;
    Threading::SafeQueue<Ptr<Message> > msgQueue;
};

//------------------------------------------------------------------------------
/**
*/
inline void
BlockingHandlerThread::SetWaitTimeout(int milliSec)
{
    n_assert(!this->IsRunning());
    this->waitTimeout = milliSec;
}

//------------------------------------------------------------------------------
/**
*/
inline int
BlockingHandlerThread::GetWaitTimeout() const
{
    return this->waitTimeout;
}

} // namespace Messaging
//------------------------------------------------------------------------------
