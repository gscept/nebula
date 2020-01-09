#pragma once
//------------------------------------------------------------------------------
/**
    @class Messaging::StaticMessageHandler
  
    Implements a simple, static message handler helper class. This separates
    the tedious message handling code into a separate class, so that the
    main class doesn't have to be polluted with message handling code.
        
    (C) 2010 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file	
*/  
#include "messaging/message.h"

//------------------------------------------------------------------------------
namespace Messaging
{
class StaticMessageHandler
{
public:
    /// dispatch a message to handling method
    template<class OBJTYPE> static void Dispatch(const Ptr<OBJTYPE>& object, const Ptr<Message>& msg);
    /// a handler method with object association
    template<class OBJTYPE, class MSGTYPE> static void Handle(const Ptr<OBJTYPE>& object, const Ptr<MSGTYPE>& msg);
    /// a handler method without object association
    template<class MSGTYPE> static void Handle(const Ptr<MSGTYPE>& msg);
};

//------------------------------------------------------------------------------
/**
*/
#define __Dispatch(OBJCLASS,OBJ,MSG) Messaging::StaticMessageHandler::Dispatch<OBJCLASS>(OBJ, MSG)
#define __Dispatcher(OBJCLASS) template<> void Messaging::StaticMessageHandler::Dispatch(const Ptr<OBJCLASS>& obj, const Ptr<Messaging::Message>& msg)
#define __Handle(OBJCLASS,MSGCLASS) if (msg->CheckId(MSGCLASS::Id)) { Messaging::StaticMessageHandler::Handle<OBJCLASS,MSGCLASS>(obj, msg.downcast<MSGCLASS>()); return; }
#define __StaticHandle(MSGCLASS) if (msg->CheckId(MSGCLASS::Id)) { Messaging::StaticMessageHandler::Handle<MSGCLASS>(msg.downcast<MSGCLASS>()); return true; }
#define __HandleByRTTI(OBJCLASS,MSGCLASS) if (msg->IsA(MSGCLASS::RTTI)) { Messaging::StaticMessageHandler::Handle<OBJCLASS,MSGCLASS>(obj, msg.downcast<MSGCLASS>()); return; }
#define __HandleUnknown(SUPERCLASS) { Messaging::StaticMessageHandler::Dispatch<SUPERCLASS>(obj.upcast<SUPERCLASS>(), msg); }
#define __Handler(OBJCLASS,MSGCLASS) template<> void Messaging::StaticMessageHandler::Handle<OBJCLASS,MSGCLASS>(const Ptr<OBJCLASS>& obj, const Ptr<MSGCLASS>& msg)
#define __StaticHandler(MSGCLASS) template<> void Messaging::StaticMessageHandler::Handle<MSGCLASS>(const Ptr<MSGCLASS>& msg)

// sends message to interface
#define __StaticSend(INTERFACECLASS, MSG) INTERFACECLASS::Instance()->Send(MSG.downcast<Messaging::Message>())
#define __StaticSendWait(INTERFACECLASS, MSG) INTERFACECLASS::Instance()->SendWait(MSG.downcast<Messaging::Message>())

// sends message to graphics entity
#define __Send(OBJ, MSG) OBJ->Send(MSG.downcast<Graphics::GraphicsEntityMessage>())

} // namespace Messaging  