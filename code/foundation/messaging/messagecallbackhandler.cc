//------------------------------------------------------------------------------
//  messagecallbackhandler.cc
//  (C) 2013 Gustav Sterbrant
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "messagecallbackhandler.h"

namespace Messaging
{
Util::Array<Ptr<Messaging::Message> >  MessageCallbackHandler::Messages;
Util::Array<Util::Delegate<const Ptr<Messaging::Message>& > >  MessageCallbackHandler::Callbacks;

//------------------------------------------------------------------------------
/**
*/
void 
MessageCallbackHandler::Update()
{
	IndexT i;
	for (i = 0; i < Messages.Size(); i++)
	{
		const Util::Delegate<const Ptr<Messaging::Message>& >& del = Callbacks[i];
		const Ptr<Messaging::Message>& msg = Messages[i];

		// if the message is handled, invoke delegate
		if (msg->Handled() || msg->DeferredHandled())
		{
            // set message to be handled, in case its just flagged as deferred handled
            msg->SetHandled(true);

			// invoke delegate
			del(msg);

			// remove callback and decrease index
			Callbacks.EraseIndex(i);
			Messages.EraseIndex(i);
			i--;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
MessageCallbackHandler::AbortCallback( const Ptr<Messaging::Message>& msg )
{
	n_assert(msg.isvalid());
	IndexT i = Messages.FindIndex(msg);
	if (i != InvalidIndex)
	{
		Callbacks.EraseIndex(i);
		Messages.EraseIndex(i);
	}	
}

} // namespace Messaging