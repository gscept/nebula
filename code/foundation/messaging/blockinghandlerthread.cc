//------------------------------------------------------------------------------
//  blockinghandlerthread.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "messaging/blockinghandlerthread.h"

namespace Messaging
{
__ImplementClass(Messaging::BlockingHandlerThread, 'BLHT', Messaging::HandlerThreadBase);

//------------------------------------------------------------------------------
/**
*/
BlockingHandlerThread::BlockingHandlerThread() :
    waitTimeout(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    This adds a new message to the thread's message queue.
*/
void
BlockingHandlerThread::AddMessage(const Ptr<Message>& msg)
{
    n_assert(msg.isvalid());
    this->msgQueue.Enqueue(msg);
}

//------------------------------------------------------------------------------
/**
    This removes a message from the thread's message queue, regardless
    of its state.
*/
void
BlockingHandlerThread::CancelMessage(const Ptr<Message>& msg)
{
    n_assert(msg.isvalid());
    this->msgQueue.EraseMatchingElements(msg);
}

//------------------------------------------------------------------------------
/**
    This method is called by Thread::Stop() after setting the 
    stopRequest event and before waiting for the thread to stop.
*/
void
BlockingHandlerThread::EmitWakeupSignal()
{
    // wake up our thread if it's waiting for messages
    this->msgQueue.Signal();
}

//------------------------------------------------------------------------------
/**
    The message processing loop.
*/
void
BlockingHandlerThread::DoWork()
{
    this->ThreadOpenHandlers();
    while (!this->ThreadStopRequested())
    {
        // wait for next message, or timeout
        bool msgHandled = false;
        if (this->waitTimeout > 0)
        {
            this->msgQueue.WaitTimeout(this->waitTimeout);
        }
        else
        {
            this->msgQueue.Wait();
        }
        
        // update state of deferred messages
        msgHandled = this->ThreadUpdateDeferredMessages();

        // process messages
        if (!this->msgQueue.IsEmpty())
        {
            Util::Array<Ptr<Message> > msgArray;
            this->msgQueue.DequeueAll(msgArray);
            msgHandled |= this->ThreadHandleMessages(msgArray);
        }

        // signal if at least one message has been handled
        if (msgHandled)
        {
            this->ThreadSignalMessageHandled();
        }

        // do per-frame update on attached handlers
        this->ThreadUpdateHandlers();
    }

    // cleanup and exit thread
    this->ThreadDiscardDeferredMessages();
    this->ThreadCloseHandlers();
}

} // namespace Messaging