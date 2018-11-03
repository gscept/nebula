//------------------------------------------------------------------------------
//  runthroughhandlerthread.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "messaging/runthroughhandlerthread.h"

namespace Messaging
{
__ImplementClass(Messaging::RunThroughHandlerThread, 'RTHT', Messaging::HandlerThreadBase);

//------------------------------------------------------------------------------
/**
*/
RunThroughHandlerThread::RunThroughHandlerThread()
{
    this->msgQueue.SetSignalOnEnqueueEnabled(false);
}

//------------------------------------------------------------------------------
/**
    This adds a new message to the thread's message queue.
*/
void
RunThroughHandlerThread::AddMessage(const Ptr<Message>& msg)
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
RunThroughHandlerThread::CancelMessage(const Ptr<Message>& msg)
{
    n_assert(msg.isvalid());
    this->msgQueue.EraseMatchingElements(msg);
}

//------------------------------------------------------------------------------
/**
    The message processing loop.
*/
void
RunThroughHandlerThread::DoWork()
{
    this->ThreadOpenHandlers();
    while (!this->ThreadStopRequested())
    {
        bool msgHandled = false;

        // do not wait for message, but yield thread on some platforms
        // (the yield is only important on the Wii, and doesn't do
        // anything on other platforms)
        Thread::YieldThread();

        // update state of deferred messages
        msgHandled = this->ThreadUpdateDeferredMessages();

        // process messages
        if (!this->msgQueue.IsEmpty())
        {
            Util::Array<Ptr<Message> > msgArray;
			msgArray.Reserve(this->msgQueue.Size());
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