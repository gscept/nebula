#pragma once
//-----------------------------------------------------------------------------
/**
    @class Messaging::Handler
    
    Message handlers are used to process a message. To handle specific
    messages, derive from Handler and overwrite the method HandleMessage().
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "messaging/message.h"

//-----------------------------------------------------------------------------
namespace Messaging
{
class Handler : public Core::RefCounted
{
    __DeclareClass(Handler);
public:
    /// constructor
    Handler();
    /// destructor
    virtual ~Handler();
    /// called once on startup 
    virtual void Open();
    /// called once before shutdown
    virtual void Close();
    /// return true if open
    bool IsOpen() const;
    /// handle a message, return true if handled
    virtual bool HandleMessage(const Ptr<Message>& msg);
    /// optional "per-frame" DoWork method for continuous handlers
    virtual void DoWork();
protected:
    bool isOpen;
};

//-----------------------------------------------------------------------------
/**
*/
inline bool
Handler::IsOpen() const
{
    return this->isOpen;
}

} // namespace Messaging
//-----------------------------------------------------------------------------
    
    