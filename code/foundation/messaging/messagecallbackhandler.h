#pragma once
//------------------------------------------------------------------------------
/**
    @class Messaging::MessageCallbackHandler
    
    Handles asynchronous message callbacks.	Allows us to perform callbacks whenever a message has been handled.
	Is primarily used for knowing whenever a get-something-from-another-thread-message has been handled.

	For example, if we send a FetchSkinList message and want to have a callback for whenever it's handled, we can simply bind the message object
	to a function using the macro.

	Example:

	Ptr<Graphics::FetchSkinList> msg = Graphics::FetchSkinList::Create();
	__Send(modelEntity, msg);	
	__SingleFireCallback(ViewerApplication, OnFetchedSkinList, this, msg.upcast<Messaging::Message>());

	Which, when msg is handled will call this->OnFetchedSkinList with the message as argument.
	It sets up a specific callback delegate, not a generic one based on message type. 
    
    (C) 2013 Gustav Sterbrant
	(C) 2013-2020 Individual contributors, see AUTHORS file
*/

//------------------------------------------------------------------------------
#include "messaging/message.h"
#include "util/dictionary.h"
#include "util/delegate.h"
#include "core/ptr.h"
namespace Messaging
{
class MessageCallbackHandler
{
public:
	/// setup a message callback
	template<class CLASS, void (CLASS::*METHOD)(const Ptr<Messaging::Message>&)> static void AddCallback(const Ptr<Messaging::Message>& msg, CLASS* obj);
	/// remove a single message callback
	static void AbortCallback(const Ptr<Messaging::Message>& msg);
	/// remove all message callbacks related to a class
	template<class CLASS> static void AbortCallbacks(CLASS* obj);
	/// update messages
	static void Update();
private:
	static Util::Array<Ptr<Messaging::Message> > Messages;
	static Util::Array<Util::Delegate<void(const Ptr<Messaging::Message>&)> > Callbacks;
}; 

//------------------------------------------------------------------------------
/**
*/
template<class CLASS>
void 
MessageCallbackHandler::AbortCallbacks( CLASS* obj )
{
	n_assert(0 != obj);
	IndexT i;
	for (i = 0; i < Callbacks.Size(); i++)
	{
		//const Util::Delegate<const Ptr<Messaging::Message>& >& callback = Callbacks[i];
		const Util::Delegate<void(const Ptr<Messaging::Message>&)>& callback = Callbacks[i];
		if (callback.GetObject<CLASS>() == obj)
		{
			Callbacks.EraseIndex(i);
			Messages.EraseIndex(i);
			i--;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
template<class CLASS, void (CLASS::*METHOD)(const Ptr<Messaging::Message>&)>
void 
MessageCallbackHandler::AddCallback(const Ptr<Messaging::Message>& msg, CLASS* obj)
{
	Util::Delegate<void(const Ptr<Messaging::Message>&)> del = Util::Delegate<void(const Ptr<Messaging::Message>&)>::FromMethod<CLASS,METHOD>(obj);
	Messages.Append(msg);
	Callbacks.Append(del);
}

//------------------------------------------------------------------------------
/**
	Define macro which sets up a single-fire callback
*/
#define __SingleFireCallback(CLASS, METHOD, OBJ, MSG) Messaging::MessageCallbackHandler::AddCallback<CLASS, &CLASS::METHOD>(MSG, OBJ);
#define __AbortSingleFireCallback(MSG) Messaging::MessageCallbackHandler::AbortCallback(MSG);
#define __AbortSingleFireCallbacks(CLASS, OBJ) Messaging::MessageCallbackHandler::AbortCallbacks<CLASS>(OBJ);
} // namespace Messaging
//------------------------------------------------------------------------------