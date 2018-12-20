#pragma once
//------------------------------------------------------------------------------
/**
    @class Messaging::Message

    Messages are packets of data which can be sent to a message port.
    This implements a universal communication mechanism within the same
    thread, different threads, or even different machines.

    Messages are implemented as normal C++ objects which can encode and
    decode themselves from and to a stream.

    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "io/binaryreader.h"
#include "io/binarywriter.h"
#include "messaging/id.h"
#include "threading/interlocked.h"

//------------------------------------------------------------------------------
/**
    Message Id macros.
*/
#define __DeclareMsgId \
public:\
    static Messaging::Id Id; \
    virtual const Messaging::Id& GetId() const;\
private:

#define __ImplementMsgId(type) \
    Messaging::Id type::Id; \
    const Messaging::Id& type::GetId() const { return type::Id; }

//------------------------------------------------------------------------------
namespace Messaging
{
class MessageReader;
class MessageWriter;
class Port;

class Message : public Core::RefCounted
{
    __DeclareClass(Message);
    __DeclareMsgId;
public:
    /// constructor
    Message();
    /// return true if message is of the given id
    bool CheckId(const Messaging::Id& id) const;
    /// encode message into a stream
    virtual void Encode(const Ptr<IO::BinaryWriter>& writer);
    /// decode message from a stream
    virtual void Decode(const Ptr<IO::BinaryReader>& reader);
    /// set the handled flag
    void SetHandled(bool b);
    /// return true if the message has been handled
    bool Handled() const;
    /// set deferred flag
    void SetDeferred(bool b);
    /// get deferred flag
    bool IsDeferred() const;
    /// set the deferred handled flag
    void SetDeferredHandled(bool b);
    /// get the deferred handled flag
    bool DeferredHandled() const;
	/// should this message be distributed over the network
	bool GetDistribute() const;
	/// enable distribution over network
	void SetDistribute(bool b);
protected:
    volatile int handled;
    bool deferred;
    bool deferredHandled;
	bool distribute;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
Message::CheckId(const Messaging::Id& id) const
{
    return (id == this->GetId());
}

//------------------------------------------------------------------------------
/**
*/
inline void
Message::SetHandled(bool b)
{
    Threading::Interlocked::Exchange(&this->handled, (int)b);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Message::Handled() const
{
    return 0 != this->handled;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Message::SetDeferred(bool b)
{
    this->deferred = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Message::IsDeferred() const
{
    return this->deferred;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Message::SetDeferredHandled(bool b)
{
    this->deferredHandled = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Message::DeferredHandled() const
{
    return this->deferredHandled;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Message::GetDistribute() const
{
	return distribute;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Message::SetDistribute(bool b)
{
	this->distribute = b;
}

} // namespace Messaging
//------------------------------------------------------------------------------
