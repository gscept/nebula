#pragma once
//------------------------------------------------------------------------------
/**
    @class Messaging::AsyncPort
  
    The AsyncPort class runs its handlers in a separate thread, so that
    message processing happens in a separate thread and doesn't block
    the main thread.
      
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#include "messaging/message.h"
#include "messaging/handler.h"
#include "messaging/handlerthreadbase.h"

//------------------------------------------------------------------------------
namespace Messaging
{
class AsyncPort : public Core::RefCounted
{
    __DeclareClass(AsyncPort);
public:
    /// constructor
    AsyncPort();
    /// destructor
    virtual ~AsyncPort();
    
    /// set pointer to handler thread object (must be derived from HandlerThreadBase)
    void SetHandlerThread(const Ptr<HandlerThreadBase>& handlerThread);
    /// get pointer to handler thread object
    const Ptr<HandlerThreadBase>& GetHandlerThread() const;
        
    /// attach a handler to the port, may be called before or after Open()
    virtual void AttachHandler(const Ptr<Handler>& h);
    /// dynamically remove a handler from the port
    virtual void RemoveHandler(const Ptr<Handler>& h);

    /// open the async port
    virtual void Open();
    /// close the async port
    virtual void Close();
    /// return true if port is open
    bool IsOpen() const;

    /// send an asynchronous message to the port
    template<class MESSAGETYPE> void Send(const Ptr<MESSAGETYPE>& msg);
    /// send a message and wait for completion
    template<class MESSAGETYPE> void SendWait(const Ptr<MESSAGETYPE>& msg);
    /// wait for a message to be handled
    template<class MESSAGETYPE> void Wait(const Ptr<MESSAGETYPE>& msg);
    /// peek a message whether it has been handled
    template<class MESSAGETYPE> bool Peek(const Ptr<MESSAGETYPE>& msg);
    /// cancel a pending message
    template<class MESSAGETYPE> void Cancel(const Ptr<MESSAGETYPE>& msg);

private:
    /// send an asynchronous message to the port
    void SendInternal(const Ptr<Message>& msg);
    /// send a message and wait for completion
    void SendWaitInternal(const Ptr<Message>& msg);
    /// wait for a message to be handled
    void WaitInternal(const Ptr<Message>& msg);
    /// peek a message whether it has been handled
    bool PeekInternal(const Ptr<Message>& msg);
    /// cancel a pending message
    void CancelInternal(const Ptr<Message>& msg);

private:
    /// clear all attached message handlers
    void ClearHandlers();

    Ptr<HandlerThreadBase> thread;
    bool isOpen;
};

//------------------------------------------------------------------------------
/**
*/
inline void
AsyncPort::SetHandlerThread(const Ptr<HandlerThreadBase>& handlerThread)
{
    n_assert(!this->IsOpen());
    this->thread = handlerThread;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<HandlerThreadBase>&
AsyncPort::GetHandlerThread() const
{
    return this->thread;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
AsyncPort::IsOpen() const
{
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
template<class MESSAGETYPE> inline void 
AsyncPort::Send(const Ptr<MESSAGETYPE>& msg)
{
    this->SendInternal((const Ptr<Messaging::Message>&)msg);
}

//------------------------------------------------------------------------------
/**
*/
template<class MESSAGETYPE> inline void 
AsyncPort::SendWait(const Ptr<MESSAGETYPE>& msg)
{
    this->SendWaitInternal((const Ptr<Messaging::Message>&)msg);
}

//------------------------------------------------------------------------------
/**
*/
template<class MESSAGETYPE> inline void 
AsyncPort::Wait(const Ptr<MESSAGETYPE>& msg)
{
    this->WaitInternal((const Ptr<Messaging::Message>&)msg);
}

//------------------------------------------------------------------------------
/**
*/
template<class MESSAGETYPE> inline bool 
AsyncPort::Peek(const Ptr<MESSAGETYPE>& msg)
{
    return this->PeekInternal((const Ptr<Messaging::Message>&)msg);
}

//------------------------------------------------------------------------------
/**
*/
template<class MESSAGETYPE> inline void 
AsyncPort::Cancel(const Ptr<MESSAGETYPE>& msg)
{
    this->CancelInternal((const Ptr<Messaging::Message>&)msg);
}

} // namespace Messaging
//------------------------------------------------------------------------------
