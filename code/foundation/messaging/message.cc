//------------------------------------------------------------------------------
//  message.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "messaging/message.h"

namespace Messaging
{
__ImplementClass(Messaging::Message, 'MSG_', Core::RefCounted);
__ImplementMsgId(Message);

//------------------------------------------------------------------------------
/**
*/
Message::Message() :
    handled(0),
    deferred(false),
    deferredHandled(false),
    distribute(true)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Message::Encode(const Ptr<IO::BinaryWriter>& writer)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Message::Decode(const Ptr<IO::BinaryReader>& reader)
{
    // empty
}

} // namespace Message
